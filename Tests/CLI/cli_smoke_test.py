#!/usr/bin/env python3
"""Smoke/regression tests for the iPlug2 CLI executable."""

from __future__ import annotations

import argparse
import json
import math
import struct
import subprocess
import sys
import tempfile
import wave
from pathlib import Path
from typing import Tuple


def run_cli(executable: Path, *args: str) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        [str(executable), *args],
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if result.returncode != 0:
        raise AssertionError(
            f"Command failed with exit code {result.returncode}: {executable} {' '.join(args)}\n"
            f"stdout:\n{result.stdout}\n"
            f"stderr:\n{result.stderr}"
        )
    return result


def assert_close(actual: float, expected: float, tolerance: float = 1.0e-9) -> None:
    if not math.isclose(actual, expected, rel_tol=tolerance, abs_tol=tolerance):
        raise AssertionError(f"Expected {expected}, got {actual}")


def write_input_wav(path: Path, sample_rate: int = 8000, frames: int = 32) -> None:
    with wave.open(str(path), "wb") as wav:
        wav.setnchannels(2)
        wav.setsampwidth(2)
        wav.setframerate(sample_rate)
        for frame in range(frames):
            sample = int(12000 * math.sin(2.0 * math.pi * frame / 8.0))
            wav.writeframes(struct.pack("<hh", sample, -sample))


def read_wav_params(path: Path) -> Tuple[int, int, int, int]:
    with wave.open(str(path), "rb") as wav:
        return wav.getnchannels(), wav.getsampwidth(), wav.getframerate(), wav.getnframes()


def test_info(executable: Path) -> None:
    info = json.loads(run_cli(executable, "--info").stdout)
    assert info["name"] == "IPlugCLITest"
    assert info["version"] == "1.0.0"
    assert info["manufacturer"] == "AcmeInc"
    assert info["type"] == "instrument"
    assert info["channels"] == {"inputs": 2, "outputs": 2}
    assert info["latency"] == 0
    assert info["midi_in"] is True
    assert info["midi_out"] is False


def test_parameters(executable: Path, work_dir: Path) -> None:
    params = json.loads(run_cli(executable, "--params").stdout)["parameters"]
    assert [param["name"] for param in params] == ["Gain", "Frequency", "Attack", "Decay"]

    gain = json.loads(run_cli(executable, "--set", "0", "42", "--param", "0").stdout)
    assert gain["name"] == "Gain"
    assert_close(gain["value"], 42.0)

    frequency = json.loads(run_cli(executable, "--set-name", "Frequency", "880", "--param", "1").stdout)
    assert frequency["name"] == "Frequency"
    assert_close(frequency["value"], 880.0)

    normalized = json.loads(run_cli(executable, "--set-norm", "0", "0.5", "--param", "0").stdout)
    assert_close(normalized["value"], 50.0)

    params_file = work_dir / "params.json"
    run_cli(executable, "--set", "0", "12.5", "--save-params", str(params_file))
    restored = json.loads(run_cli(executable, "--load-params", str(params_file), "--param", "0").stdout)
    assert_close(restored["value"], 12.5)


def test_generated_audio_outputs(executable: Path, work_dir: Path) -> None:
    txt_file = work_dir / "sine.txt"
    run_cli(executable, "--sr", "1000", "--sine", "100", "10", "--output-txt", str(txt_file))
    samples = [float(line) for line in txt_file.read_text(encoding="utf-8").splitlines()]
    assert len(samples) == 10

    wav_file = work_dir / "sine.wav"
    run_cli(executable, "--sr", "1000", "--sine", "100", "10", "--output", str(wav_file))
    channels, sample_width, sample_rate, frames = read_wav_params(wav_file)
    assert (channels, sample_width, sample_rate, frames) == (2, 3, 1000, 10)


def test_file_processing(executable: Path, work_dir: Path) -> None:
    input_wav = work_dir / "input.wav"
    output_wav = work_dir / "output.wav"
    write_input_wav(input_wav)

    run_cli(executable, "--input", str(input_wav), "--process-file", "--output", str(output_wav))

    channels, sample_width, sample_rate, frames = read_wav_params(output_wav)
    assert (channels, sample_width, sample_rate, frames) == (2, 3, 8000, 32)


def test_midi_render(executable: Path, work_dir: Path) -> None:
    txt_file = work_dir / "midi.txt"
    run_cli(
        executable,
        "--sr",
        "1000",
        "--midi",
        "69",
        "100",
        "0",
        "5",
        "--render",
        "10",
        "--output-txt",
        str(txt_file),
    )
    samples = [float(line) for line in txt_file.read_text(encoding="utf-8").splitlines()]
    assert len(samples) == 10


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--executable", type=Path, required=True)
    args = parser.parse_args()

    executable = args.executable.resolve()
    if not executable.exists():
        raise AssertionError(f"CLI executable does not exist: {executable}")

    with tempfile.TemporaryDirectory(prefix="iplug-cli-test-") as tmp:
        work_dir = Path(tmp)
        test_info(executable)
        test_parameters(executable, work_dir)
        test_generated_audio_outputs(executable, work_dir)
        test_file_processing(executable, work_dir)
        test_midi_render(executable, work_dir)

    print("iPlug2 CLI smoke tests passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())

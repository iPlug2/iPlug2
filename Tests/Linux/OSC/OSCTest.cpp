#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <thread>

#include "IPlugOSC.h"

using namespace iplug;
using Clock = std::chrono::steady_clock;

static void Check(bool condition, const char* message)
{
  if (!condition)
  {
    fprintf(stderr, "FAIL: %s\n", message);
    std::exit(1);
  }
}

class Socket
{
public:
  Socket()
  {
    mFD = socket(AF_INET, SOCK_DGRAM, 0);
    Check(mFD != INVALID_SOCKET, "create UDP socket");
    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    Check(bind(mFD, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0, "bind UDP socket");
    socklen_t size = sizeof(address);
    Check(getsockname(mFD, reinterpret_cast<sockaddr*>(&address), &size) == 0, "read UDP port");
    mPort = ntohs(address.sin_port);
    SET_SOCK_BLOCK(mFD, false);
  }

  ~Socket() { close(mFD); }
  Socket(const Socket&) = delete;
  Socket& operator=(const Socket&) = delete;
  int Port() const { return mPort; }

  bool Read()
  {
    char packet[MAX_OSC_MSG_LEN];
    const auto size = recv(mFD, packet, sizeof(packet), 0);
    if (size < 0)
    {
      Check(errno == EAGAIN || errno == EWOULDBLOCK, "read UDP socket");
      return false;
    }
    OscMessageRead message(packet, static_cast<int>(size));
    Check(message.GetMessage() && strcmp(message.GetMessage(), "/probe") == 0, "OSC wire message");
    const int* value = message.PopIntArg(false);
    Check(value && *value == 42, "OSC wire argument");
    return true;
  }

private:
  int mFD = INVALID_SOCKET;
  int mPort = 0;
};

static void Send(OSCSender& sender)
{
  OscMessageWrite message;
  Check(message.PushWord("/probe"), "write OSC address");
  message.PushIntArg(42);
  sender.SendOSCMessage(message);
}

static void Until(const std::function<bool()>& done, const std::function<void()>& tick)
{
  const auto deadline = Clock::now() + std::chrono::seconds(2);
  while (!done())
  {
    Check(Clock::now() < deadline, "OSC operation finishes before timeout");
    tick();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

class DeviceProbe final : public OSCInterface
{
public:
  DeviceProbe()
  {
    WDL_String log;
    mInput = CreateReceiver(log, 0);
    Check(mInput != nullptr, "create ephemeral OSC receiver");
    sockaddr_in address {};
    socklen_t size = sizeof(address);
    Check(getsockname(mInput->mSendSocket, reinterpret_cast<sockaddr*>(&address), &size) == 0,
      "read OSC receiver port");
    mPort = ntohs(address.sin_port);
    mOutput = CreateSender(log, "127.0.0.1", mPort);
    Check(mOutput != nullptr, "create second OSC device");
  }

  int Port() const { return mPort; }
  int Calls() const { return mCalls; }

  void OnOSCMessage(OscMessageRead&) override
  {
    ++mCalls;
    Check(mCalls == 1, "recursive polling does not deliver a nested callback");
    ProcessOSC();
    RemoveDevice(mInput);
    mInput = nullptr;
    RemoveDevice(mOutput);
    mOutput = nullptr;
  }

private:
  OSCDevice* mInput = nullptr;
  OSCDevice* mOutput = nullptr;
  int mPort = 0;
  int mCalls = 0;
};

int main()
{
  const auto owner = std::this_thread::get_id();
  Socket firstOutput;
  Socket secondOutput;
  {
    OSCSender sender("127.0.0.1", firstOutput.Port());
    Send(sender);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    Check(!firstOutput.Read(), "no worker flushes queued sends");
    sender.ProcessOSC();
    Until([&] { return firstOutput.Read(); }, [] {});
    sender.SetDestination("127.0.0.1", secondOutput.Port());
    Send(sender);
    sender.ProcessOSC();
    Until([&] { return secondOutput.Read(); }, [] {});
    Check(!firstOutput.Read(), "destination change stops sending to old port");
  }

  int firstPort, secondPort;
  {
    Socket firstReservation;
    Socket secondReservation;
    firstPort = firstReservation.Port();
    secondPort = secondReservation.Port();
  }
  std::atomic<int> received {0};
  {
    OSCReceiver* pReceiver = nullptr;
    OSCReceiver receiver(firstPort, nullptr, [&](OscMessageRead& message) {
      Check(std::this_thread::get_id() == owner, "receive callback runs on the polling thread");
      Check(message.GetMessage() && strcmp(message.GetMessage(), "/probe") == 0, "receive OSC address");
      const int* value = message.PopIntArg(false);
      Check(value && *value == 42, "receive OSC argument");
      ++received;
      if (received == 1)
      {
        pReceiver->SetReceivePort(secondPort);
        pReceiver->ProcessOSC();
      }
    });
    pReceiver = &receiver;
    OSCSender sender("127.0.0.1", firstPort);
    Send(sender);
    sender.ProcessOSC();
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    Check(received == 0, "receive callback waits for explicit polling");
    Until([&] { return received == 1; }, [&] { receiver.ProcessOSC(); });

    sender.SetDestination("127.0.0.1", secondPort);
    Send(sender);
    sender.ProcessOSC();
    Until([&] { return received == 2; }, [&] { receiver.ProcessOSC(); });
  }
  {
    OSCReceiver reopened(secondPort, nullptr, [&](OscMessageRead&) { ++received; });
    OSCSender sender("127.0.0.1", secondPort);
    Send(sender);
    sender.ProcessOSC();
    Until([&] { return received == 3; }, [&] { reopened.ProcessOSC(); });
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(250));
  Check(received == 3, "no callbacks after destruction");

  {
    DeviceProbe probe;
    OSCSender sender("127.0.0.1", probe.Port());
    Send(sender);
    sender.ProcessOSC();
    Until([&] { return probe.Calls() == 1; }, [&] { probe.ProcessOSC(); });
    probe.ProcessOSC();
  }

#if defined OS_MAC && !defined OS_LINUX
  {
    Socket output;
    OSCSender sender("127.0.0.1", output.Port());
    Send(sender);
    Until([&] { return output.Read(); }, [] {
      CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.02, true);
    });
  }
#endif
  puts("OSC polling, device changes, reentrancy and lifecycle checks passed");
  return 0;
}

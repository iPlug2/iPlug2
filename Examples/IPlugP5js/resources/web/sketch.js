/*
 * @name Passing Shader Uniforms
 * @description Uniforms are the way in which information is passed from p5 to the shader.
 * <br> To learn more about using shaders in p5.js: <a href="https://itp-xstory.github.io/p5js-shaders/">p5.js Shaders</a>
 */

 // this variable will hold our shader object
 let theShader;
 // Must match the kOctave enum index and the octave count in IPlugP5js.h.
 const kParamOctave = 1;
 const kNumOctaves = 5;
 let lastOctave = -1;

 function preload(){
   // load the shader
   theShader = loadShader('uniforms.vert', 'uniforms.frag');
 }

 function setup() {
   // shaders require WEBGL mode to work
   const size = getSketchSize();
   const canvas = createCanvas(size.width, size.height, WEBGL);
   canvas.parent('sketch');
   noStroke();
 }

 function draw() {
   // shader() sets the active shader with our shader
   shader(theShader);

   const mouseShape = getMouseShape();
   sendOctave(getOctaveForShape(mouseShape));

   // lets send the resolution, mouse, and time to our shader
   // before sending mouse + time we modify the data so it's more easily usable by the shader
   theShader.setUniform('resolution', [width, height]);
   theShader.setUniform('mouse', mouseShape);
   theShader.setUniform('time', frameCount * 0.01);

   // In WEBGL mode the origin is the centre of the canvas.
   rect(-width / 2, -height / 2, width, height);
 }

function windowResized() {
  const size = getSketchSize();
  resizeCanvas(size.width, size.height);
}

function getSketchSize() {
  const sketch = document.getElementById('sketch');
  const bounds = sketch ? sketch.getBoundingClientRect() : null;

  // Clamp to a min of 1: bounds and window.inner* can be 0 before layout/in headless contexts.
  return {
    width: Math.max(1, Math.round(bounds?.width || window.innerWidth || 1)),
    height: Math.max(1, Math.round(bounds?.height || window.innerHeight || 1))
  };
}

function getMouseShape() {
  const normalizedX = Math.max(0, Math.min(1, mouseX / Math.max(1, width)));
  return normalizedX * 7;
}

function getOctaveForShape(shape) {
  return Math.min(kNumOctaves - 1, Math.floor((shape / 7) * kNumOctaves));
}

function sendOctave(octave) {
  if (octave === lastOctave || typeof SPVFUI !== 'function') {
    return;
  }

  lastOctave = octave;
  // SPVFUI expects a 0..1 normalised value; iPlug maps it back to the enum index on the C++ side.
  const normalizedOctave = octave / (kNumOctaves - 1);

  if (typeof BPCFUI === 'function') {
    BPCFUI(kParamOctave);
  }

  SPVFUI(kParamOctave, normalizedOctave);

  if (typeof EPCFUI === 'function') {
    EPCFUI(kParamOctave);
  }
}

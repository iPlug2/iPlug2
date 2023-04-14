// this is an attribute sent to the shader by p5
// it contains all of our vertex position information
// it is a vec3, meaning it contains x, y, and z data
// attribute signals that this is a global variable sent by the sketch
// it is read only, meaning it cannot be changed directly (you can copy it though)
// attributes exist in vertex shaders only
attribute vec3 aPosition;

// all shaders have one main function
// the vertex shader requires there to be a vec4 output called gl_Position
void main() {
   
  // copy the position data into a vec4, using 1.0 as the w component
  vec4 positionVec4 = vec4(aPosition, 1.0);

  // scale the rect by two, and move it to the center of the screen
  // if we don't do this, it will appear with its bottom left corner in the center of the sketch
  // try commenting this line out to see what happens
  positionVec4.xy = positionVec4.xy * 2.0 - 1.0;

  // send the vertex information on to the fragment shader
  gl_Position = positionVec4;
}

#version 120

uniform sampler1D transforms;
uniform int numTransforms;

attribute vec2 position;
attribute vec2 texCoords;
attribute float spriteNumber;

varying vec2 texCoordsOut;

// Given a world position (for this lab, bottom-left is (0, 0), top-right of window is (1280, 720)),
// convert it to be in [-1, 1]^2, where y = -1 is the bottom and x = -1 is the left side.
vec2 cameraPos(vec2 worldPos) {
    return vec2(
    (worldPos.x / 1280.0) * 2.0 - 1.0,
    (worldPos.y / 720.0) * 2.0 - 1.0
);
}

void main() {
    // set gl_Position and texCoordsOut...
    // TODO
    float transformUV = (spriteNumber + 0.5) / float(numTransforms);

    vec4 transformData = texture1D(transforms, transformUV);

    vec2 scale = transformData.xy;
    vec2 translation = transformData.zw;

    vec2 worldPos = position * scale + translation;

    gl_Position = vec4(cameraPos(worldPos), 0.0, 1.0);

    texCoordsOut = texCoords;
}

#version 460 core

layout(location = 0) in vec2 pixelPosition;
layout(location = 1) in vec2 baseUVInput;
layout(location = 2) in vec2 maskUVInput;
layout(location = 3) in vec4 color0Input;

uniform vec2 viewport;

layout(location = 0) out vec2 baseUV;
layout(location = 1) out vec2 maskUV;
layout(location = 2) out vec4 color0;

void main() {
    gl_Position = vec4(pixelPosition.x / viewport.x * 2.0 - 1.0,
                       1.0 - pixelPosition.y / viewport.y * 2.0,
                       0.0, 1.0);
    baseUV = baseUVInput;
    maskUV = maskUVInput;
    color0 = color0Input;
}

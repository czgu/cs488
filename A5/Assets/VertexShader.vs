#version 330

uniform mat4 PV;
uniform mat4 M;

in vec4 position;
out vec4 texcoord;

void main() {
    texcoord = position;
	gl_Position = PV * M * vec4(position.xyz, 1.0);
}

#version 330

// uniform vec3 colour;

in vec4 texcoord;
out vec4 fragColor;

void main() {
	fragColor = vec4(texcoord.w / 8.0, texcoord.w / 16.0, texcoord.w / 32.0 ,1.0);
}

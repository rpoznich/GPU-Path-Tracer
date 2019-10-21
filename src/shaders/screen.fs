R"zzz(#version 330 core

in vec2 texcoord;

uniform sampler2D tex;

out vec4 fragment_color;

void main(void) {
  
  fragment_color = texture(tex, texcoord);
}
)zzz"
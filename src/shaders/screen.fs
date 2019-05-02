R"zzz(#version 330 core

/* This comes interpolated from the vertex shader */
in vec2 texcoord;

/* The texture we are going to sample */
uniform sampler2D tex;

out vec4 fragment_color;

void main(void) {
  /* Well, simply sample the texture */
  fragment_color = texture(tex, texcoord);
}
)zzz"
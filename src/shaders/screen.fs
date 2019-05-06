R"zzz(#version 330 core

/* This comes interpolated from the vertex shader */
in vec2 texcoord;

/* The texture we are going to sample */
uniform sampler2D tex;

out vec4 fragment_color;

void main(void) {
  /* Well, simply sample the texture */
  fragment_color = vec4(vec3(texture(tex, texcoord)),1.0);
}
)zzz"
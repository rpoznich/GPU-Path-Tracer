R"zzz(#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;
const float BOX_SIZE = 0.5f;
uniform mat4 projection;
uniform mat4 model;
uniform mat4 view;
uniform vec4 light_position;
in vec4 vs_light_direction[];
in vec4 vs_camera_direction[];
in vec4 vs_normal[];
in vec2 vs_uv[];
out vec4 face_normal;
out vec4 light_direction;
out vec4 camera_direction;
out vec4 world_position;
out vec4 vertex_normal;
out vec2 uv_coords;
flat out vec3 corners;
void main() {
	int n = 0;
	vec3 a = gl_in[0].gl_Position.xyz;
	vec3 b = gl_in[1].gl_Position.xyz;
	vec3 c = gl_in[2].gl_Position.xyz;
	vec3 u = normalize(b - a);
	vec3 v = normalize(c - a);
	face_normal = normalize(vec4(normalize(cross(u, v)), 0.0));
	float min_x = min(c.x,min(a.x,b.x));
	float min_y = min(c.y,min(a.y,b.y));
	float min_z = min(c.z,min(a.z,b.z));
	float max_x = max(c.x,max(a.x,b.x));
	float max_y = max(c.y,max(a.y,b.y));
	float max_z = max(c.z,max(a.z,b.z));
	corners = vec3(min_x, max_y, min_z);
	for (n = 0; n < gl_in.length(); n++) {
		light_direction = normalize(vs_light_direction[n]);
		camera_direction = normalize(vs_camera_direction[n]);
		world_position = gl_in[n].gl_Position;
		vertex_normal = vs_normal[n];
		gl_Position = projection * view * model * gl_in[n].gl_Position;
		EmitVertex();
	}
	EndPrimitive();
}
)zzz"

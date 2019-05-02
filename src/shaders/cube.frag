R"zzz(#version 410 core

const float BOX_SIZE = 1.0f;
in vec4 face_normal;
in vec4 light_direction;
in vec4 world_position;
in vec2 uv_coords;
out vec4 fragment_color;
uniform int seeds[512];
uniform float random_angle[256];
flat in vec3 corners;

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}
float smoothNoise(float x, float y)
{
   //get fractional part of x and y
   float fractX = x - int(x);
   float fractY = y - int(y);

   //wrap around
   int x1 = int(x) % 256;
   int y1 = int(y) % 256;

   //neighbor values
   int x2 = (x1  - 1 + 256) %256;
   int y2 = (y1  - 1 + 256) % 256;

   //smooth the noise with bilinear interpolation

   float value = 0.0;
   value += fractX     * fractY     * float(seeds[x1+seeds[y1]]);
   value += (1.0 - fractX) * fractY     * float(seeds[x2+seeds[y1]]);
   value += fractX    * (1.0 - fractY) * float(seeds[x1+seeds[y2]]);
   value += (1.0 - fractX) * (1.0 - fractY) * float(seeds[x2+seeds[y2]]);

   return value;
}



vec2 getGradientVector(float patch_x, float patch_y, float patch_z)
{
	int x = int(patch_x) % 256;
	int z = int(patch_z) % 256;
	int y = int(patch_y) % 256;
	x = x < 0 ? 256 - abs(x) : x;
	z = z < 0 ? 256 - abs(z) : z;
	y = y < 0 ? 256 - abs(y) : y;
	float theta = random_angle[seeds[x+seeds[y+seeds[z]]]];
	return normalize(vec2(sin(theta),cos(theta)));
}

vec3 hueToRGB(float hue)
{
    float r = abs(hue * 6.0 - 3.0) - 1.0;
    float g = 2.0 - abs(hue * 6.0 - 2.0);
    float b = 2.0 - abs(hue * 6.0 - 4.0);
    return clamp(vec3(r, g, b), 0.0, 1.0);
}

vec3 HSLToRGB(vec3 hsl)
{
    vec3 rgb = hueToRGB(hsl.x);
    float c = (1.0 - abs(2.0 * hsl.z - 1.0)) * hsl.y;
    return (rgb - 0.5) * c + hsl.z;
}

void main()
{
	vec3 color;
	float patch_x = corners[0];
	float patch_y = corners[1];
	float patch_z = corners[2];
	float u = abs(world_position.x - patch_x) / BOX_SIZE;
	float v = abs(world_position.z - patch_z) / BOX_SIZE;
	if(abs(face_normal[0]) > 0.0 || abs(face_normal[2]) > 0.0)
	{
		if(abs(face_normal[2]) > 0.0)
			u = abs(world_position.x - corners[0]) / BOX_SIZE;
		else
			u = abs(world_position.z - corners[2]) / BOX_SIZE;
		v = abs(world_position.y - corners[1]) / BOX_SIZE;
	}
	if(abs(face_normal[0]) > 0.0 || abs(face_normal[2]) > 0.0) //bounding sides
	{
	
		vec2 d,c,b,a;
		if(abs(face_normal[2]) > 0.0 )
		{
			d = getGradientVector(patch_x, patch_y, patch_z);
			c = getGradientVector(patch_x + BOX_SIZE, patch_y, patch_z);
			b = getGradientVector(patch_x + BOX_SIZE, patch_y + BOX_SIZE, patch_z);
			a = getGradientVector(patch_x, patch_y + BOX_SIZE, patch_z);
		} else 
		{
			d = getGradientVector(patch_x, patch_y, patch_z);
			c = getGradientVector(patch_x, patch_y, patch_z  + BOX_SIZE);
			b = getGradientVector(patch_x, patch_y + BOX_SIZE, patch_z + BOX_SIZE);
			a = getGradientVector(patch_x, patch_y + BOX_SIZE, patch_z);
		}
		vec2 _c = normalize(vec2(u,v)- vec2(1,0));
		vec2 _b = normalize(vec2(u,v)- vec2(1,1));
		vec2 _d = normalize(vec2(u,v)- vec2(0,0));
		vec2 _a = normalize(vec2(u,v)- vec2(0,1));
		float noise =   (sin(((1-v)*((1-u)*dot(a, _a)+ u * dot(b,_b)) + v * ((1-u)*dot(d,_d)+ u * dot(c,_c)))) + 1 ) / 2.0;
		float grass_noise =  ((smoothNoise(u * 255.0, 0)/ 255.0) ) * 0.2;
		float stone_noise =  ((smoothNoise(u * 255.0, 0)/ 255.0) ) * 0.1;
		if(patch_y > BOX_SIZE * 4)
		{
			if(v > grass_noise)
			{
				color = HSLToRGB(vec3(38.0/356.0,0.73,(0.14 + (0.34 - 0.14)*(noise))*(1-v)));		
			} else 
			{
				color = vec3(94,157,52) / 255.0;
			}

		} else {
			color = HSLToRGB(vec3(11.0/356.0,0.01,0.5+ (0.7 - 0.5)*(noise)));
			if(v < stone_noise)
			{
				color = vec3(199,199,199) / 255.0;
			}
		}

	}
	else {
		vec2 d = getGradientVector(patch_x, patch_y, patch_z);
		vec2 c =  getGradientVector(patch_x + BOX_SIZE, patch_y, patch_z);
		vec2 b = getGradientVector(patch_x + BOX_SIZE, patch_y, patch_z + BOX_SIZE);
		vec2 a = getGradientVector(patch_x, patch_y, patch_z + BOX_SIZE);
		vec2 _c = normalize(vec2(u,v)- vec2(1,0));
		vec2 _b = normalize(vec2(u,v)- vec2(1,1));
		vec2 _d = normalize(vec2(u,v)- vec2(0,0));
		vec2 _a = normalize(vec2(u,v)- vec2(0,1));
		float noise =   (sin(((1-v)*((1-u)*dot(a, _a)+ u * dot(b,_b)) + v * ((1-u)*dot(d,_d)+ u * dot(c,_c)))) + 1 ) / 2.0;
		if(patch_y >BOX_SIZE * 4)
			color = HSLToRGB(vec3(89.0/356.0,0.97,0.38 + (0.43 - 0.38)*(noise)));
		else
			color = HSLToRGB(vec3(47.0/356.0,0.01  + (0.10 - 0.01)*(noise),0.51));
		
	}
	float dot_nl = dot(normalize(light_direction), normalize(face_normal));

	dot_nl = 1.0f;

	fragment_color = vec4(clamp(dot_nl * color, 0.0, 1.0),1.0);


}
)zzz"

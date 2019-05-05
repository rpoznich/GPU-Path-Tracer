R"zzz(#version 430 core
#extension GL_ARB_texture_cube_map_array : require

layout(binding = 1, rg16f) uniform imageCubeArray photonMaps;
uniform samplerCubeArray cubeMaps;
uniform vec3 camera_position;
uniform vec3 ray00;
uniform vec3 ray01;
uniform vec3 ray10;
uniform vec3 ray11;

in vec2 texcoord;

out vec4 fragment_color;
struct box {
  vec3 min;
  vec3 max;
  vec3 kd_;
  vec3 ks_;
  bool ref;
};
#define MAX_SCENE_BOUNDS 1000000.0
#define NUM_BOXES 21
#define EPSILON 0.000001

const box boxes[] = box[21](
  /* The ground */
  box(vec3(43, 0.0, 43), vec3(44, 0.1, 44), vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), false),
  box(vec3(44, 0.0, 43), vec3(45, 0.1, 44), vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), false),
  box(vec3(45, 0.0, 43), vec3(46, 0.1, 44), vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), false),
  box(vec3(46, 0.0, 43), vec3(47, 0.1, 44), vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), false),
  box(vec3(47, 0.0, 43), vec3(48, 0.1, 44), vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), false),
  box(vec3(43, 0.0, 44), vec3(44, 0.1, 45), vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), false),
  box(vec3(44, 0.0, 44), vec3(45, 0.1, 45), vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), false),
  box(vec3(45, 0.0, 44), vec3(46, 0.1, 45), vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), false),
  box(vec3(46, 0.0, 44), vec3(47, 0.1, 45), vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), false),
  box(vec3(47, 0.0, 44), vec3(48, 0.1, 45), vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), false),
  box(vec3(43, 0.0, 45), vec3(44, 0.1, 46), vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), false),
  box(vec3(44, 0.0, 45), vec3(45, 0.1, 46), vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), false),
  box(vec3(45, 0.0, 45), vec3(46, 0.1, 46), vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), false),
  box(vec3(46, 0.0, 45), vec3(47, 0.1, 46), vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), false),
  box(vec3(47, 0.0, 45), vec3(48, 0.1, 46), vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), false),
  box(vec3(43, 0.0, 46), vec3(44, 0.1, 47), vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), false),
  box(vec3(44, 0.0, 46), vec3(45, 0.1, 47), vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), false),
  box(vec3(45, 0.0, 46), vec3(46, 0.1, 47), vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), false),
  box(vec3(46, 0.0, 46), vec3(47, 0.1, 47), vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), false),
  box(vec3(47, 0.0, 46), vec3(48, 0.1, 47), vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), false),
  /* Box in the middle */
  box(vec3(45, 0.5, 45), vec3(46, 1.5, 46), vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), false)
);

struct isect
{
  float tMin;
  float tMax;
  int bi;
  int face;
  bool ref;
  vec3 normal;
};


bool intersectBox(vec3 origin, vec3 dir, const box b, out isect i) 
{
        vec3 tMin = (b.min - origin) / dir;
        vec3 tMax = (b.max - origin) / dir;
        vec3 t1 = min(tMin, tMax);
        vec3 t2 = max(tMin, tMax);
        i.tMin = max(max(t1.x, t1.y), t1.z);
        i.tMax = min(min(t2.x, t2.y), t2.z);
        if(i.tMax >= 0.0 && i.tMin < i.tMax)
        {
          vec3 hit = origin + dir * i.tMin;
          if (dir.x > 0.0 && abs(hit.x - b.min.x) < EPSILON){
            i.face= 1;
            i.normal = vec3(-1.0, 0.0, 0.0);
          }
          else if (dir.x < 0.0 && abs(hit.x - b.max.x) < EPSILON){
            i.face =  0;
            i.normal =vec3(1.0, 0.0, 0.0);
          }
          else if (dir.y > 0.0 && abs(hit.y - b.min.y) < EPSILON){
            i.face =  3;
            i.normal = vec3(0.0, -1.0, 0.0);
          }
          else if (dir.y < 0.0 && abs(hit.y - b.max.y) < EPSILON){
             i.face =  2;
             i.normal = vec3(0.0, 1.0, 0.0);
          }
          else if (dir.z > 0.0 && abs(hit.z - b.min.z) < EPSILON){
             i.face =  5;
             i.normal = vec3(0.0, 0.0, -1.0);
          }
          else if (dir.z < 0.0 && abs(hit.z - b.max.z) < EPSILON){
             i.face =  4;
             i.normal = vec3(0.0, 0.0, 1.0);
          }
          else{
            i.face = -1;
            return false;
          }

          return true;
        }

        return false;
}


vec2 intersectBox(vec3 origin, vec3 dir, const box b) {
  vec3 tMin = (b.min - origin) / dir;
  vec3 tMax = (b.max - origin) / dir;
  vec3 t1 = min(tMin, tMax);
  vec3 t2 = max(tMin, tMax);
  float tNear = max(max(t1.x, t1.y), t1.z);
  float tFar = min(min(t2.x, t2.y), t2.z);
  return vec2(tNear, tFar);
}

bool intersectBoxes(vec3 origin, vec3 dir, out isect intersection) {
  float smallest = MAX_SCENE_BOUNDS;
  bool found = false;
  for (int i = 0; i < NUM_BOXES; i++) {
    isect temp;
    if (intersectBox(origin, dir, boxes[i], temp) && temp.tMin < smallest) {
      intersection = temp;
      smallest = intersection.tMin;
      found = true;
      intersection.ref = boxes[i].ref;
      intersection.bi = i;
    }
  }
  return found;
}

vec2 texCoordForFace(vec3 hit, const box b, int fIndex) {
  if (fIndex == 0) {
    vec2 res = (hit.zy - b.min.zy) / (b.max.zy - b.min.zy);
    return vec2(1.0 - res.x, 1.0 - res.y);
  } else if (fIndex == 1) {
    vec2 res = (hit.zy - b.min.zy) / (b.max.zy - b.min.zy);
    return vec2(res.x, 1.0 - res.y);
    
  } else if (fIndex == 2) {
    vec2 res = (hit.xz - b.min.xz) / (b.max.xz - b.min.xz);
    return res;
  } else if (fIndex == 3) {
    vec2 res = (hit.xz - b.min.xz) / (b.max.xz - b.min.xz);
    return res;
    
  } else if (fIndex == 4) {
    vec2 res = (hit.xy - b.min.xy) / (b.max.xy - b.min.xy);
    return vec2(res.x, 1.0 - res.y);
  } else {
    vec2 res = (hit.xy - b.min.xy) / (b.max.xy - b.min.xy);
    return vec2(1.0 - res.x, 1.0 - res.y);
  }
}


vec3 trace(vec3 origin, vec3 dir) {
  bool keepGoing = true;
  vec3 color = vec3(0.0, 0.0, 0.0);
  int depth = 10;
  float attenuate_factor = 1;
  ivec3 size = imageSize(photonMaps);
  while(depth > 0)
  {
    isect i;
    if (intersectBoxes(origin, dir, i)) {
      vec3 hitPoint = origin + dir * i.tMin;
      box b = boxes[i.bi];
      vec3 unitPoint = (hitPoint - b.min) / (b.max - b.min) - vec3(0.5,0.5,0.5);
      ivec3 index = ivec3(texCoordForFace(hitPoint,b,i.face) * size.xy, i.bi*6 + i.face);
      //float r= imageLoad(photonMaps,index).r;
      float r = texture(cubeMaps, vec4(unitPoint, float(i.bi))).r ;
      if(!i.ref)
      {
        color += vec3(r,r,r);
        break;
      } else 
      {
        color += vec3(r,r,r);
        attenuate_factor *= 0.5;
        origin += (i.tMin-0.0001) * dir;
        dir = reflect(dir, i.normal);
      }
      depth--;
    } else 
      break;
  }
  return color;
}

void main(void) {
  vec2 pos = texcoord;
  vec3 dir = normalize(mix(mix(ray00, ray01, pos.y), mix(ray10, ray11, pos.y), pos.x));
  fragment_color = vec4(clamp(trace(camera_position, dir),0.0,1.0),1.0f);
}
)zzz"
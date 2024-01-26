#pragma once
#include "mathUtil.h"
#include "color.h"
#include <fstream>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <map>

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <memory>

void extract_face_vertices_and_normals(const std::string& face_string,
                                        std::vector<int>& vertex_indices,
                                        std::vector<int>& normal_indices);
  

struct Material
{
    Color diffuse;
    Color specular;
    float k_ambient, k_diffuse, k_specular;
    float specular_exponent;
    float eta;
    float alpha;
    Material(Color diffuse, Color specular, float k_ambient, float k_diffuse, float k_specular, float intensity, float eta, float alpha): 
        diffuse(diffuse), specular(specular), k_ambient(k_ambient), k_diffuse(k_diffuse), k_specular(k_specular), specular_exponent(intensity), eta(eta), alpha(alpha) {}
    Material() : diffuse(0, 0, 0), specular(0, 0, 0), k_ambient(0), k_diffuse(0), k_specular(0), specular_exponent(0), eta(0), alpha(0) {}
};
enum LightType {POINT, DIRECTIONAL, SPOTLIGHT};
struct Light
{
    Point pos;
    Color color;
    Light(Point pos, Color color) : pos(pos), color(color) {}
    Light() : pos(0, 0, 0), color(1, 1, 1) {}
    LightType type = POINT;
    Vec3 GetDirection(const Point& target) {
        if (type == LightType::DIRECTIONAL) {
            return Vec3::Normalize(-pos);
        }
        else {
            return Vec3::Normalize(target - pos);
        }
    }
};
enum class ObjectType {SPHERE, FACE};

class Object {
public:
    Material material;
    int id;
    int texture_index = -1;
    ObjectType type;
    Object(const Material& mat) : material(mat) {
        static int _id = 0;
        this->id = _id++;
    }
    virtual Vec3 GetNormal(Point p = Vec3(0, 0, 0)) = 0;
    virtual ~Object() = default;
};

class Sphere : public Object {
public:
    Vec3 pos{0, 0, 0};
    float radius{0};
    Sphere(const Vec3& pos, float radius, const Material& mat)
        : Object(mat), pos(pos), radius(radius) {
            this->type = ObjectType::SPHERE;
        }
    Sphere() : Object(Material()) {
        this->type = ObjectType::SPHERE;
    }
    Sphere(const Sphere& other) : Object(other.material), pos(other.pos), radius(other.radius) {
        this->type = ObjectType::SPHERE;
        this->texture_index = other.texture_index;
    }
    Vec3 GetNormal(Point p = Vec3(0, 0, 0)) override {
        if (p == Vec3(0, 0, 0)) {
            return Vec3(0, 0, 0);
        }
        return Vec3::Normalize(p - pos) / radius;
    }
    // Other sphere-specific methods and properties
};
struct Vertex
{
    Point pos;
    Material material;
    Vec3 normal;
    std::pair<float, float> texture_coord;
    Vertex(Point pos, Vec3 _normal) : pos(pos),normal(normal) {}
    Vertex(Point pos) : pos(pos), normal(0, 0, 0), texture_coord(-1, -1) {}
    Vertex() : pos(0, 0, 0) {}
};
class Face : public Object {
public:
    Vertex v0, v1, v2;
    bool has_normals = false;
    Face(const Vertex& v0, const Vertex& v1, const Vertex& v2, const Material& mat)
        : Object(mat), v0(v0), v1(v1), v2(v2), has_normals(false){
            this->type = ObjectType::FACE;
        }
    Face (const Vertex& v0, const Vertex& v1, const Vertex& v2, const Material& mat, bool has_normals)
        : Object(mat), v0(v0), v1(v1), v2(v2), has_normals(has_normals) {
            this->type = ObjectType::FACE;
        }
    Face() : Object(Material()) {
        this->type = ObjectType::FACE;
    }
    Face(const Face& other) : Object(other.material), v0(other.v0), v1(other.v1), v2(other.v2), has_normals(other.has_normals) {
        this->type = ObjectType::FACE;
        this->texture_index = other.texture_index;
    }
    Vec3 GetNormal(Point p = Vec3(0, 0, 0)) override {
        Vec3 v0v1 = v1.pos - v0.pos;
        Vec3 v0v2 = v2.pos - v0.pos;
        return Vec3::Normalize(Vec3::Cross(v0v1, v0v2));
    }
};
struct InputFileData
{ 
    std::pair<int, int> imsize;
    Point eye;
    Vec3 viewdir;
    Vec3 updir;
    float hfov;
    Color bkgcolor;
    float index_of_refraction;
    std::vector<Sphere> spheres;
    std::vector<Light> lights;
    std::vector<Vertex> vertex_arrays;
    std::vector<Face> faces;
   Material triangle_material;
   std::vector<Vec3> vertex_normals;
   std::vector<Image> texture;
   std::vector<std::pair<int, int>> texture_coords;
   std::vector<std::shared_ptr<Object>> objects;
};


InputFileData get_input(std::string inputfile) {
      
    std::ifstream file(inputfile);
     //if the file is not found return error
    if(!file) {
        std::cerr << "Unable to open file " << inputfile << std::endl;
               exit(1);
    }
    InputFileData res;
    std::string line;

    while (file.good())
    {
        std::getline(file, line);
        if(line[0] == '#') {
            continue;
        }
        //check if the is new line
        if(line == "" || line == " " || line == "\n" || line == "\t" || line == "\r" ) {
                    continue;
        }
        std::istringstream iss(line);
        std::string key;
        iss >> key;
        if(key == "imsize")
        {
            //check if the input is valid
            if(!(iss >> res.imsize.first >> res.imsize.second)) {
                std::cerr << "Invalid imsize(w, h) in input file" << std::endl;
                //print ouut the contents of the file
                std::cout<< line << std::endl;
                exit(1);
            }
        }
        else if(key == "eye")
        {
            //check if the input is valid
            if(!(iss >> res.eye.x >> res.eye.y >> res.eye.z)) {
               std::cerr << "Invalid eye(x, y,z) in input file " << std::endl;
                exit(1);
            }
        }
        else if(key == "viewdir")
        {
            //check if the input is valid
            if(!(iss >> res.viewdir.x >> res.viewdir.y >> res.viewdir.z)) {
                std::cerr << "Invalid viewdir(x, y,z) in input file" << std::endl;
                exit(1);
            }
        }
        else if(key == "updir")
        {
            //check if the input is valid
            if(!(iss >> res.updir.x >> res.updir.y >> res.updir.z)) {
                std::cerr << "Invalid updir(x, y,z) in input file" << std::endl;
                exit(1);
            }
        }
        else if(key == "hfov")
        {
            //check if the input is valid
            if(!(iss >> res.hfov)) {
                std::cerr << "Invalid hfov in input file" << std::endl;
                exit(1);
            }
        }
        else if(key == "bkgcolor")
        {
            //check if the input is valid
            if(!(iss >> res.bkgcolor.R >> res.bkgcolor.G >> res.bkgcolor.B >> res.index_of_refraction)) {
                std::cerr << "Invalid bkgcolor(r, g, b) in input file" << std::endl;
                exit(1);
            }
        }
        //light
        else if(key == "light")
        {
            auto light = Light();
            Point pos;
            Color color;
            //check if the input is valid
            int pointLight;
            if(!(iss >> pos.x >> pos.y >> pos.z >>pointLight>> color.R >> color.G >> color.B)) {
                std::cerr << "Invalid light in input file" << std::endl;
                exit(1);
            }
            light = Light(pos, color);
            if(pointLight == 1)
            {
                light.type = LightType::POINT;
            }
            else
            {
                light.type = LightType::DIRECTIONAL;
            }
            res.lights.push_back(light);
        }
        else if( key == "v")
        {
            Point p;
            if(!(iss >> p.x >> p.y >> p.z)) {
                std::cerr << "Invalid vertex(x, y, z) in input file" << std::endl;
                exit(1);
            }
            Vertex v(p);
            res.vertex_arrays.push_back(v);
        }
        else if( key == "vn")
        {
            Point p;
            if(!(iss >> p.x >> p.y >> p.z)) {
                std::cerr << "Invalid normla(x, y, z) in input file" << std::endl;
                exit(1);
            }
            res.vertex_normals.push_back(Vec3::Normalize(p));
        }
        else if( key == "vt")
        {
            float x, y;
            if(!(iss >> x >> y)) {
                std::cerr << "Invalid texture coordinate(x, y) in input file" << std::endl;
                exit(1);
            }
            res.texture_coords.push_back(std::make_pair(x, y));
        }
        else if(key == "mtlcolor")
        {
            Material material;
            if(!(iss >> material.diffuse.R>> material.diffuse.G>> material.diffuse.B>> material.specular.R>> material.specular.G>> 
                    material.specular.B>> material.k_ambient>> material.k_diffuse>> material.k_specular>> material.specular_exponent >>material.alpha>>material.eta))
            {
                std::cerr << "Invalid material in input file" << std::endl;
                exit(1);
            }
            res.triangle_material = material;
            //every sphere after this line will have this material until another line that is not sphere is read
            //read the next line unless it is eof is not 
            while (file.good())
            {
                std::getline(file, line);
                std::istringstream iss(line);
                std::string key;
                iss >> key;
                //check if the line is empty
                if(line == "" || line == " " || line == "\n" || line == "\t" || line == "\r") {
                    continue;
                }
                if (key == "sphere")
                {
                    Vec3 pos;
                    float radius;
                    //check if the input is valid
                    if(!(iss >> pos.x >> pos.y >> pos.z >> radius)) {
                        std::cerr << "Invalid sphere(x, y, z, r) in input file" << std::endl;
                        exit(1);
                    }
                    if(radius <= 0) {
                        std::cerr << "Invalid sphere radius in input file" << std::endl;
                        exit(1);
                    }
                    //Create new sphere and push it to object list
                    Sphere sphere(pos, radius, material);
                    res.objects.push_back(std::move(std::make_shared<Sphere>(sphere)));
                }
                else if( key == "v")
                {
                    Point p;
                    if(!(iss >> p.x >> p.y >> p.z)) {
                        std::cerr << "Invalid vertex(x, y, z) in input file" << std::endl;
                        exit(1);
                    }
                    Vertex v(p);
                    res.vertex_arrays.push_back(v);
                }
                else if( key == "vn")
                {
                    Point p;
                    if(!(iss >> p.x >> p.y >> p.z)) {
                        std::cerr << "Invalid normla(x, y, z) in input file" << std::endl;
                        exit(1);
                    }
                    res.vertex_normals.push_back(Vec3::Normalize(p));
                }
                else if( key == "vt")
                {
                    float x, y;
                    if(!(iss >> x >> y)) {
                        std::cerr << "Invalid texture coordinate(x, y) in input file" << std::endl;
                        exit(1);
                    }
                    res.texture_coords.push_back(std::make_pair(x, y));
                }
                else if (key == "f")
                {
                    Face f;
                    // read the rest of the line into a string
                    std::string face_string;
                    std::getline(iss, face_string);
                    std::istringstream face_iss(face_string);
                    // check if the face is in the v//vn format
                    if (face_string.find("//") != std::string::npos)
                    {
                        int v1, v2, v3, vn1, vn2, vn3;
                        char slash;
                        if (!(face_iss >> v1 >> slash >> slash >> vn1 >> v2 >> slash >> slash >> vn2 >> v3 >> slash >> slash >> vn3))
                        {
                            std::cerr << "Invalid face(v//vn v//vn v//vn) in input file" << std::endl;
                            exit(1);
                        }
                        f.v0 = res.vertex_arrays[v1 - 1];
                        f.v1 = res.vertex_arrays[v2 - 1];
                        f.v2 = res.vertex_arrays[v3 - 1];
                        f.v0.normal = res.vertex_normals[vn1 - 1];
                        f.v1.normal= res.vertex_normals[vn2 - 1];
                        f.v2.normal = res.vertex_normals[vn3 - 1];
                        f.v0.material = material;
                        f.has_normals = true;
                        res.faces.push_back(f);
                    }
                    else if (face_string.find("/") != std::string::npos)
                    {
                        int v1, v2, v3, vt1, vt2, vt3, vn1, vn2, vn3;
                        int num_slashes = std::count(face_string.begin(), face_string.end(), '/');
                        
                        if (num_slashes == 3)
                        {
                            //v/vt v/vt v/vt
                            char slash;
                            std::istringstream face_iss(face_string);
                            face_iss >> v1 >> slash >> vt1 >> v2 >> slash >> vt2 >> v3 >> slash >> vt3;

                            if (face_iss.fail())
                            {
                                std::cerr << "Invalid face(v/vt v/vt v/vt) in input file" << std::endl;
                                exit(1);
                            }

                            f.v0 = res.vertex_arrays[v1 - 1];
                            f.v1 = res.vertex_arrays[v2 - 1];
                            f.v2 = res.vertex_arrays[v3 - 1];
                            //check if the texture coordinates are valid
                            if (res.texture_coords.empty())
                            {
                                std::cerr << "Texture coordinates found but no texture specified" << std::endl;
                                exit(1);
                            }
                            f.v0.texture_coord = res.texture_coords[vt1 - 1];
                            f.v1.texture_coord = res.texture_coords[vt2 - 1];
                            f.v2.texture_coord = res.texture_coords[vt3 - 1];
                            f.v0.material = material;
                            if(res.texture.empty())
                            {
                                std::cerr << "Texture coordinates found but no texture specified" << std::endl;
                                exit(1);
                            }
                            res.faces.push_back(f);
                        }
                        else if (num_slashes == 6)
                        {
                            //v/vt/vn v/vt/vn v/vt/vn
                            char slash;
                            std::istringstream face_iss(face_string);
                            face_iss >> v1 >> slash >> vt1 >> slash >> vn1
                                    >> v2 >> slash >> vt2 >> slash >> vn2
                                    >> v3 >> slash >> vt3 >> slash >> vn3;

                            if (face_iss.fail())
                            {
                                std::cerr << "Invalid face(v/vt/vn v/vt/vn v/vt/vn) in input file" << std::endl;
                                exit(1);
                            }

                            f.v0 = res.vertex_arrays[v1 - 1];
                            f.v1 = res.vertex_arrays[v2 - 1];
                            f.v2 = res.vertex_arrays[v3 - 1];
                            //check if the texture coordinates are valid
                            if (res.texture_coords.empty())
                            {
                                std::cerr << "Texture coordinates found but no texture specified" << std::endl;
                                exit(1);
                            }
                            f.v0.texture_coord = res.texture_coords[vt1 - 1];
                            f.v1.texture_coord = res.texture_coords[vt2 - 1];
                            f.v2.texture_coord = res.texture_coords[vt3 - 1];
                            if(res.texture.empty())
                            {
                                std::cerr << "Texture coordinates found but no texture specified" << std::endl;
                                exit(1);
                            }
                            if (res.vertex_normals.empty())
                            {
                                std::cerr << "Vertex normals found but no normals specified" << std::endl;
                                exit(1);
                            }
                            f.v0.normal = res.vertex_normals[vn1 - 1];
                            f.v1.normal = res.vertex_normals[vn2 - 1];
                            f.v2.normal = res.vertex_normals[vn3 - 1];
                            f.v0.material = material;
                            f.has_normals = true;
                            res.faces.push_back(f);
                        }
                        else
                        {
                            std::cerr << "Invalid face format in input file: "<< num_slashes << std::endl;
                            exit(1);
                        }
                    }
                    else
                    {
                        int v1, v2, v3;
                        if (!(face_iss >> v1 >> v2 >> v3))
                        {
                            std::cerr << "Invalid face(v v v) in input file" << std::endl;
                            exit(1);
                        }
                        f.v0 = res.vertex_arrays[v1 - 1];
                        f.v1 = res.vertex_arrays[v2 - 1];
                        f.v2 = res.vertex_arrays[v3 - 1];
                        f.v0.material = material;
                        res.faces.push_back(f);
                    }
                    res.objects.push_back(std::make_shared<Face>(f));
                }
                //read texture
                else if(key == "texture")
                {
                    //if res.texture is not initialized, initialize it
                    if(res.texture.empty())
                    {
                        res.texture = std::vector<Image>();
                    }
                    // extract the filename
                    std::string filename;
                    iss >> filename;
                    auto texture = Image::ReadPPM(filename);
                    res.texture.push_back(texture);
                    //get the index of the texture in the vector
                    int index = res.texture.size() - 1;
                    //every sphere/triangle after this line will have this texture until another line that is not sphere/triangle is read
                    while (file.good())
                    {   
                        std::getline(file, line);
                        std::istringstream iss(line);
                        //check if the line is empty
                        if(line == "" || line == " " || line == "\n" || line == "\t" || line == "\r") {
                            continue;
                        }
                        std::string key;
                        iss >> key;
                       
                        if (key == "sphere")
                        {
                            Vec3 pos;
                            float radius;
                            //check if the input is valid
                            if(!(iss >> pos.x >> pos.y >> pos.z >> radius)) {
                                std::cerr << "Invalid sphere(x, y, z, r) in input file" << std::endl;
                                exit(1);
                            }
                            if(radius <= 0) {
                                std::cerr << "Invalid sphere radius in input file" << std::endl;
                                exit(1);
                            }
                            auto s = Sphere(pos, radius, material);
                            s.texture_index = index;
                            res.spheres.push_back(s);
                            res.objects.push_back(std::make_shared<Sphere>(s));
                        }
    
                        else if( key == "v")
                        {
                            Point p;
                            if(!(iss >> p.x >> p.y >> p.z)) {
                                std::cerr << "Invalid vertex(x, y, z) in input file" << std::endl;
                                exit(1);
                            }
                            Vertex v(p);
                            res.vertex_arrays.push_back(v);
                        }
                        else if( key == "vn")
                        {
                            Point p;
                            if(!(iss >> p.x >> p.y >> p.z)) {
                                std::cerr << "Invalid normla(x, y, z) in input file" << std::endl;
                                exit(1);
                            }
                            res.vertex_normals.push_back(Vec3::Normalize(p));
                        }
                        else if( key == "vt")
                        {
                            float x, y;
                            if(!(iss >> x >> y)) {
                                std::cerr << "Invalid texture coordinate(x, y) in input file" << std::endl;
                                exit(1);
                            }
                            res.texture_coords.push_back(std::make_pair(x, y));
                        }
                        else if (key == "f")
                        {
                            Face f;
                            // read the rest of the line into a string
                            std::string face_string;
                            std::getline(iss, face_string);
                            std::istringstream face_iss(face_string);
                            // check if the face is in the v//vn format
                            if (face_string.find("//") != std::string::npos)
                            {
                                int v1, v2, v3, vn1, vn2, vn3;
                                char slash;
                                if (!(face_iss >> v1 >> slash >> slash >> vn1 >> v2 >> slash >> slash >> vn2 >> v3 >> slash >> slash >> vn3))
                                {
                                    std::cerr << "Invalid face(v//vn v//vn v//vn) in input file" << std::endl;
                                    exit(1);
                                }
                               
                                f.v0 = res.vertex_arrays[v1 - 1];
                                f.v1 = res.vertex_arrays[v2 - 1];
                                f.v2 = res.vertex_arrays[v3 - 1];
                                //check if there are normals
                                if(res.vertex_normals.empty())
                                {
                                    std::cerr << "No normals in input file" << std::endl;
                                    exit(1);
                                }
                                else{
                                    f.v0.normal = res.vertex_normals[vn1 - 1];
                                    f.v1.normal= res.vertex_normals[vn2 - 1];
                                    f.v2.normal = res.vertex_normals[vn3 - 1];
                                    f.has_normals = true;
                                }
                                f.v0.material = material;
                                res.faces.push_back(f);
                            }
                            else if (face_string.find("/") != std::string::npos)
                            {
                                int v1, v2, v3, vt1, vt2, vt3, vn1, vn2, vn3;
                                int num_slashes = std::count(face_string.begin(), face_string.end(), '/');
                                
                                if (num_slashes == 3)
                                {
                                    //v/vt v/vt v/vt
                                    char slash;
                                    std::istringstream face_iss(face_string);
                                    face_iss >> v1 >> slash >> vt1 >> v2 >> slash >> vt2 >> v3 >> slash >> vt3;

                                    if (face_iss.fail())
                                    {
                                        std::cerr << "Invalid face(v/vt v/vt v/vt) in input file" << std::endl;
                                        exit(1);
                                    }

                                    f.v0 = res.vertex_arrays[v1 - 1];
                                    f.v1 = res.vertex_arrays[v2 - 1];
                                    f.v2 = res.vertex_arrays[v3 - 1];
                                    f.v0.texture_coord = res.texture_coords[vt1 - 1];
                                    f.v1.texture_coord = res.texture_coords[vt2 - 1];
                                    f.v2.texture_coord = res.texture_coords[vt3 - 1];
                                    f.v0.material = material;
                                    f.texture_index = index;
                                    res.faces.push_back(f);
                                }
                                else if (num_slashes == 6)
                                {
                                    //v/vt/vn v/vt/vn v/vt/vn
                                    char slash;
                                    std::istringstream face_iss(face_string);
                                    face_iss >> v1 >> slash >> vt1 >> slash >> vn1
                                            >> v2 >> slash >> vt2 >> slash >> vn2
                                            >> v3 >> slash >> vt3 >> slash >> vn3;

                                    if (face_iss.fail())
                                    {
                                        std::cerr << "Invalid face(v/vt/vn v/vt/vn v/vt/vn) in input file" << std::endl;
                                        exit(1);
                                    }

                                    f.v0 = res.vertex_arrays[v1 - 1];
                                    f.v1 = res.vertex_arrays[v2 - 1];
                                    f.v2 = res.vertex_arrays[v3 - 1];
                                    f.v0.texture_coord = res.texture_coords[vt1 - 1];
                                    f.v1.texture_coord = res.texture_coords[vt2 - 1];
                                    f.v2.texture_coord = res.texture_coords[vt3 - 1];
                                    f.v0.normal = res.vertex_normals[vn1 - 1];
                                    f.v1.normal = res.vertex_normals[vn2 - 1];
                                    f.v2.normal = res.vertex_normals[vn3 - 1];
                                    f.v0.material = material;
                                    f.texture_index = index;
                                    f.has_normals = true;
                                    res.faces.push_back(f);
                                }
                                else
                                {
                                    std::cerr << "Invalid face format in input file" << std::endl;
                                    exit(1);
                                }
                            }
                            else
                            {
                                int v1, v2, v3;
                                if (!(face_iss >> v1 >> v2 >> v3))
                                {
                                    std::cerr << "Invalid face(v v v) in inpdsdsut file" << std::endl;
                                    exit(1);
                                }
                                f.v0 = res.vertex_arrays[v1 - 1];
                                f.v1 = res.vertex_arrays[v2 - 1];
                                f.v2 = res.vertex_arrays[v3 - 1];
                                f.v0.material = material;
                                res.faces.push_back(f);
                            }
                            f.texture_index = index;
                            res.objects.push_back(std::make_shared<Face>(f));
                        }
                        else
                        {
                            file.seekg(-line.length()-1, std::ios_base::cur);
                            break;
                        }
                    }
                }
                else {
                    file.seekg(-line.length()-1, std::ios_base::cur);
                    break;
                }
            }
        }
    }
    //check if input file is valid i.e imsize, eye,viewdir, hfov, updir, bkgcolor,mtlcolor sphere are present
    if(res.imsize.first < 0 || res.imsize.second < 0)
    {
        std::cerr << "Invalid imsize(w, h) in input file" << std::endl;
        exit(1);
    }
    return res;
}





void extract_face_vertices_and_normals(const std::string& face_string,
                                        std::vector<int>& vertex_indices,
                                        std::vector<int>& normal_indices) {
    std::istringstream iss(face_string);
    int vertex, normal;
    char slash;

    while (iss >> vertex >> slash >> slash >> normal) {
        if (normal < 0) {
            normal += vertex_indices.size() + 1;
        }
        normal_indices.push_back(normal);
        if (vertex < 0) {
            vertex += normal_indices.size() + 1;
        }
        vertex_indices.push_back(vertex);
    }
}



#if 1
static void input_print_helper(InputFileData input)
{
    std::cout << "imsize: " << input.imsize.first << " " << input.imsize.second << std::endl;
    std::cout << "eye: " << input.eye << std::endl;
    std::cout << "viewdir: " << input.viewdir << std::endl;
    std::cout << "hfov: " << input.hfov << std::endl;
    std::cout << "updir: " << input.updir << std::endl;
    std::cout << "bkgcolor: " << input.bkgcolor << input.index_of_refraction << std::endl;
    //print the lights
    for(size_t i = 0; i < input.lights.size(); i++)
    {
        std::cout << "light: " << input.lights[i].pos << " " << input.lights[i].color << std::endl;
    }

    // for(size_t i = 0; i < input.spheres.size(); i++)
    // {
    //     std::cout<<"mtlcolor: "<<input.spheres[i].material.diffuse<<" "<<input.spheres[i].material.specular<<" "<<input.spheres[i].material.k_ambient<<" "<<input.spheres[i].material.k_diffuse
    //                             <<" "<<input.spheres[i].material.k_specular<<" "<<input.spheres[i].material.specular_exponent<<" "<<input.spheres[i].material.alpha<<" "<<input.spheres[i].material.eta<<std::endl;
    //     std::cout << "sphere: " << input.spheres[i].pos << " " << input.spheres[i].radius << std::endl;
    // }

    //print the spheres
    for(size_t i = 0; i < input.objects.size(); i++)
    {
        if(input.objects[i]->type == ObjectType::SPHERE)
        {
            std::cout<<"mtlcolor: "<<input.objects[i]->material.diffuse<<" "<<input.objects[i]->material.specular<<" "<<input.objects[i]->material.k_ambient<<" "<<input.objects[i]->material.k_diffuse
                                    <<" "<<input.objects[i]->material.k_specular<<" "<<input.objects[i]->material.specular_exponent<<" "<<input.objects[i]->material.alpha<<" "<<input.objects[i]->material.eta<<std::endl;
            auto sphere = dynamic_cast<Sphere*>(input.objects[i].get());
            std::cout << "sphere: " << sphere->pos << " " << sphere->radius;
            if(sphere->texture_index != -1)
            {
                std::cout << " " << input.texture[sphere->texture_index].name;
            }
            std::cout << std::endl;
        }
    }

    if(input.faces.size() > 0)
    {
        auto v0 = input.faces[0].v0.material;
    std::cout<<"mtlcolor: "<<v0.diffuse<<" "<<v0.specular<<" "<<v0.k_ambient<<" "<<v0.k_diffuse
                                <<" "<<v0.k_specular<<" "<<v0.specular_exponent << " "<<v0.alpha << " "<<v0.eta << std::endl;
    }
    for(size_t i = 0; i < input.vertex_arrays.size(); i++)
    {
        std::cout << "v  " << input.vertex_arrays[i].pos << std::endl;
    }
    for(size_t i = 0; i < input.vertex_normals.size(); i++)
    {
        std::cout << "vn  " << input.vertex_normals[i] << std::endl;
    }
    for(size_t i = 0; i < input.texture_coords.size(); i++)
    {
        std::cout << "vt  " << input.texture_coords[i].first << " " << input.texture_coords[i].second << std::endl;
    }

    //print the faces
    for(size_t i = 0; i < input.objects.size(); i++)
    {
        if(input.objects[i]->type == ObjectType::FACE)
        {
            //cast to face
            Face* face = dynamic_cast<Face*>(input.objects[i].get());
            std::cout << "f ";
            if(face->v0.normal != Vec3(0,0,0))
            {
                std::cout << face->v0.pos << "//" << face->v0.normal << " ";
                std::cout << face->v1.pos << "//" << face->v1.normal << " ";
                std::cout << face->v2.pos << "//" << face->v2.normal << std::endl;
                continue;
            }
            std::cout << face->v0.pos << " ";
            std::cout << face->v1.pos << " ";
            std::cout << face->v2.pos;
            if(face->texture_index != -1)
            {
                std::cout << " " << input.texture[face->texture_index].name;
            }
            std::cout << std::endl;
        }
    }
    //print textures
    for(size_t i = 0; i < input.texture.size(); i++)
    {
        std::cout<<"texture: "<<input.texture[i].name<<std::endl;
    }
}
#endif
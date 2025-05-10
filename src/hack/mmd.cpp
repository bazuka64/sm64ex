#include "pch.h"
#include "mmd.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <windows.h>

template <typename T>
void operator<<(T &value, struct BinaryReader &br);

struct BinaryReader {

    std::vector<char> data;
    char *ptr;

    BinaryReader(const fs::path &path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file)
            throw 0;
        data.resize(file.tellg());
        file.seekg(0);
        file.read(data.data(), data.size());
        ptr = data.data();
    }

    void operator+=(int size) {
        ptr += size;
    }

    void Read(std::wstring &wstr) {
        memcpy(wstr.data(), ptr, wstr.size() * 2);
        ptr += wstr.size() * 2;
    }

    int ReadSize(int size) {
        switch (size) {
            case 1: {
                char value;
                value << *this;
                return value;
            }
            case 2: {
                short value;
                value << *this;
                return value;
            }
            case 4: {
                int value;
                value << *this;
                return value;
            }
            default:
                throw 0;
        }
    }

    unsigned int ReadUSize(int size) {
        switch (size) {
            case 1: {
                unsigned char value;
                value << *this;
                return value;
            }
            case 2: {
                unsigned short value;
                value << *this;
                return value;
            }
            case 4: {
                unsigned int value;
                value << *this;
                return value;
            }
            default:
                throw 0;
        }
    }
};

template <typename T>
void operator<<(T &value, BinaryReader &br) {
    value = *(T *) br.ptr;
    br.ptr += sizeof(T);
}

VMDAnimation::VMDAnimation(const fs::path &path) {
    BinaryReader br(path);
    br += 50;

    // bone
    {
        int bone_count;
        bone_count << br;
        for (int i = 0; i < bone_count; i++) {

            std::array<char, 15> name; // shift jis
            name << br;
            int size = MultiByteToWideChar(CP_ACP, 0, name.data(), -1, 0, 0);
            std::wstring wstr(size, 0);
            MultiByteToWideChar(CP_ACP, 0, name.data(), -1, wstr.data(), size);
            wstr.pop_back(); // erase null terminator

            int frame;
            frame << br;
            glm::vec3 trans;
            trans << br;
            trans.z *= -1;
            glm::quat rot;
            rot << br;
            rot.x *= -1;
            rot.y *= -1;
            br += 64; // bezier

            bone_map[wstr].push_back(BoneFrame{ frame, trans, rot });

            max_frame = std::max(max_frame, frame);
        }

        // sort
        for (auto &[name, bone_frames] : bone_map) {
            std::sort(bone_frames.begin(), bone_frames.end(), [](BoneFrame &a, BoneFrame &b) { return a.frame < b.frame; });
        }
    }

    // morph
    {
        int morph_count;
        morph_count << br;
        for (int i = 0; i < morph_count; i++) {

            std::array<char, 15> name; // shift jis
            name << br;
            int size = MultiByteToWideChar(CP_ACP, 0, name.data(), -1, 0, 0);
            std::wstring wstr(size, 0);
            MultiByteToWideChar(CP_ACP, 0, name.data(), -1, wstr.data(), size);
            wstr.pop_back(); // erase null terminator

            int frame;
            frame << br;
            float weight;
            weight << br;

            morph_map[wstr].push_back(MorphFrame{ frame, weight });
        }

        // sort
        for (auto &[name, morph_frames] : morph_map) {
            std::sort(morph_frames.begin(), morph_frames.end(), [](MorphFrame &a, MorphFrame &b) { return a.frame < b.frame; });
        }
    }
}

MMDModel::MMDModel(const fs::path &path) {
    BinaryReader br(path);
    struct Header {
        char encode;        // 0:UTF16 1:UTF8
        char additional_uv; // 0-4
        char vertex;        // 1,2,4 unsigned
        char texture;       // 1,2,4 signed
        char material;      // 1,2,4 signed
        char bone;          // 1,2,4 signed
        char morph;         // 1,2,4 signed
        char rigidbone;     // 1,2,4 signed
    };
    Header header;
    {
        br += 9;
        header << br;
        // model info
        for (int i = 0; i < 4; i++) {
            int size;
            size << br;
            br += size;
        }
    }
    struct Vertex {
        glm::vec3 pos;
        glm::vec2 uv;
        glm::ivec4 bones;
        glm::vec4 weights;
    };
    std::vector<Vertex> vertices;
    {
        int vertex_count;
        vertex_count << br;
        vertices.resize(vertex_count);
        for (Vertex &vertex : vertices) {
            vertex.pos << br;
            vertex.pos.z *= -1;
            br += 12; // normal
            vertex.uv << br;
            br += 16 * header.additional_uv;
            char skinning_type; // 0:BDEF1 1:BDEF2 2:BDEF4 3:SDEF
            skinning_type << br;
            switch (skinning_type) {
                case 0: // BDEF1
                    vertex.bones[0] = br.ReadSize(header.bone);
                    vertex.weights[0] = 1;
                    break;
                case 1: // BDEF2
                case 3: // SDEF
                    for (int i = 0; i < 2; i++) {
                        vertex.bones[i] = br.ReadSize(header.bone);
                    }
                    vertex.weights[0] << br;
                    vertex.weights[1] = 1 - vertex.weights[0];
                    if (skinning_type == 3) // SDEF
                        br += 12 + 12 + 12;
                    break;
                case 2: // BDEF4
                    for (int i = 0; i < 4; i++) {
                        vertex.bones[i] = br.ReadSize(header.bone);
                    }
                    vertex.weights << br;
                    break;
            }
            br += 4; // edge
        }
    }
    std::vector<int> indices;
    {
        int index_count;
        index_count << br;
        indices.resize(index_count);
        for (int &index : indices) {
            index = br.ReadUSize(header.vertex);
        }
    }
    // build buffers
    {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, pos));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, uv));
        glEnableVertexAttribArray(2);
        glVertexAttribIPointer(2, 4, GL_INT, sizeof(Vertex), (void *) offsetof(Vertex, bones));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, weights));

        glGenBuffers(1, &morph_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, morph_vbo);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, false, 0, 0);

        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &ubo);

        glBindVertexArray(0);
    }
    // texture
    {
        int texture_count;
        texture_count << br;
        textures.resize(texture_count);
        fs::path dir = path.parent_path();
        for (GLuint &texture : textures) {
            int size;
            size << br;
            std::wstring filename(size / 2, 0);
            br.Read(filename);
            fs::path tex_path = dir / filename;
            int x, y;
            unsigned char *data = stbi_load(tex_path.string().c_str(), &x, &y, 0, 4);
            if (!data)
                throw 0;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            stbi_image_free(data);
        }
    }
    // material
    {
        int material_count;
        material_count << br;
        materials.resize(material_count);
        int offset = 0;
        for (Material &material : materials) {
            for (int i = 0; i < 2; i++) {
                int size;
                size << br;
                br += size;
            }
            br += 16 + 12 + 4 + 12 + 1 + 16 + 4;
            material.texture_index = br.ReadSize(header.texture);
            br += header.texture + 1; // sphere texture
            char toon_flag;
            toon_flag << br;
            if (toon_flag == 0) {
                br += header.texture;
            } else {
                br += 1;
            }
            // memo
            int size;
            size << br;
            br += size;
            material.index_count << br;
            material.index_offset = offset;
            offset += material.index_count;
        }
    }
    // bone
    {
        int bone_count;
        bone_count << br;
        bones.resize(bone_count);
        FinalTransform.resize(bone_count);
        for (Bone &bone : bones) {
            int size;
            size << br;
            bone.name.resize(size / 2);
            br.Read(bone.name);
            size << br;
            br += size;

            bone.pos << br;
            bone.pos.z *= -1;
            bone.InverseBindPose = glm::translate(glm::mat4(1), -bone.pos);

            int parent = br.ReadSize(header.bone);
            if (parent != -1) {
                bone.parent = &bones[parent];
                bone.ParentOffset = bone.pos - bone.parent->pos;
                bone.parent->children.push_back(&bone);
            } else {
                bone.ParentOffset = bone.pos;
            }
            br += 4; // 変形階層

            struct BoneFlag {
                unsigned short connection : 1;
                unsigned short rotatable : 1;
                unsigned short movable : 1;
                unsigned short visible : 1;
                unsigned short operatable : 1;
                unsigned short ik : 1;
                unsigned short nop : 1;
                unsigned short append_local : 1;
                unsigned short append_rotate : 1;
                unsigned short append_translate : 1;
                unsigned short fixed_axis : 1;
                unsigned short local_axis : 1;
                unsigned short deform_after_physic : 1;
                unsigned short external_parent_deform : 1;
            };
            BoneFlag flag;
            flag << br;

            if (flag.connection == 0)
                br += 12;
            else
                br += header.bone;
            if (flag.append_rotate || flag.append_translate) {
                int parent = br.ReadSize(header.bone);
                bone.append_parent = &bones[parent];
                br += 4; // weight
                append_bones.push_back(&bone);
            }
            if (flag.fixed_axis)
                br += 12;
            if (flag.local_axis)
                br += 12 + 12;
            if (flag.external_parent_deform)
                br += 4;

            if (flag.ik) {
                ik_bones.push_back(&bone);
                int target = br.ReadSize(header.bone);
                bone.target = &bones[target];
                br += 4 + 4;
                int link_count;
                link_count << br;
                bone.links.resize(link_count);
                for (int i = 0; i < link_count; i++) {
                    int link = br.ReadSize(header.bone);
                    bone.links[i] = &bones[link];
                    char angle_limit;
                    angle_limit << br;
                    if (angle_limit) {
                        br += 12 + 12;
                    }
                }
            }
        }
    }
    // morph
    {
        int morph_count;
        morph_count << br;
        morphs.resize(morph_count);
        morph_pos.resize(vertices.size());
        for (Morph &morph : morphs) {

            int size;
            size << br;
            morph.name.resize(size / 2);
            br.Read(morph.name);
            size << br;
            br += size;

            br += 1; // 操作パネル
            morph.type << br;

            int offset_count;
            offset_count << br;

            switch (morph.type) {
                case 0: // group
                    morph.group_morphs.resize(offset_count);
                    for (GroupMorph &gm : morph.group_morphs) {
                        gm.index = br.ReadSize(header.morph);
                        gm.weight << br;
                    }
                    break;
                case 1: // vertex
                    morph.vertex_morphs.resize(offset_count);
                    for (VertexMorph &vm : morph.vertex_morphs) {
                        vm.index = br.ReadUSize(header.vertex);
                        vm.offset << br;
                        vm.offset.z *= -1;
                    }
                    break;
                case 2: // bone
                    br += offset_count * (header.bone + 12 + 16);
                    break;
                case 3: // uv
                case 4: // additional uv 1
                case 5: // additional uv 2
                case 6: // additional uv 3
                case 7: // additional uv 4
                    br += offset_count * (header.vertex + 16);
                    break;
                case 8: // material
                    br += offset_count * (header.material + 1 + 16 + 12 + 4 + 12 + 16 + 4 + 16 + 16 + 16);
                    break;
            }
        }
    }
}

void MMDModel::Draw(VMDAnimation *anim) {

    extern unsigned short gAreaUpdateCounter;
    float anim_frame = gAreaUpdateCounter % (anim->max_frame + 1);

    // animation
    for (Bone &bone : bones) {

        if (anim->bone_map.count(bone.name) > 0) {
            std::vector<VMDAnimation::BoneFrame> &bone_frames = anim->bone_map[bone.name];

            auto it = std::find_if(bone_frames.begin(), bone_frames.end(), [anim_frame](VMDAnimation::BoneFrame &bf) { return bf.frame > anim_frame; });

            glm::vec3 Translation;
            glm::quat Rotation;

            if (it == bone_frames.begin()) {
                VMDAnimation::BoneFrame &bf = *it;
                Translation = bf.trans;
                Rotation = bf.rot;
            } else if (it == bone_frames.end()) {
                VMDAnimation::BoneFrame &bf = *std::prev(it);
                Translation = bf.trans;
                Rotation = bf.rot;
            } else {
                VMDAnimation::BoneFrame &bf0 = *std::prev(it);
                VMDAnimation::BoneFrame &bf1 = *it;
                float t = (anim_frame - bf0.frame) / (bf1.frame - bf0.frame);
                Translation = glm::mix(bf0.trans, bf1.trans, t);
                Rotation = glm::slerp(bf0.rot, bf1.rot, t);
            }

            bone.LocalTransform = glm::translate(glm::mat4(1), bone.ParentOffset + Translation);
            bone.LocalTransform *= glm::mat4(Rotation);
        } else {
            // non animation
            bone.LocalTransform = glm::translate(glm::mat4(1), bone.ParentOffset);
        }

        if (bone.parent) {
            bone.GlobalTransform = bone.parent->GlobalTransform * bone.LocalTransform;
        } else {
            bone.GlobalTransform = bone.LocalTransform;
        }
    }

    // ik
    for (Bone *ptr : ik_bones) {

        Bone &ik_bone = *ptr;
        glm::vec3 ik_pos = ik_bone.GlobalTransform[3];
        Bone &target_bone = *ik_bone.target;
        glm::vec3 target_pos = target_bone.GlobalTransform[3];

        if (ik_bone.links.size() == 1) {
            Bone &foot_bone = *ik_bone.links[0];
            OneBoneIK(foot_bone, target_pos, ik_pos);
        } else if (ik_bone.links.size() == 2) {

            Bone &knee_bone = *ik_bone.links[0];
            glm::vec3 knee_pos = knee_bone.GlobalTransform[3];

            Bone &leg_bone = *ik_bone.links[1];
            glm::vec3 leg_pos = leg_bone.GlobalTransform[3];

            float a = glm::distance(ik_pos, leg_pos);
            float b = glm::distance(target_pos, knee_pos);
            float c = glm::distance(knee_pos, leg_pos);

            if (a < b + c) {
                // cosine rule
                float A = glm::acos((b * b + c * c - a * a) / (2 * b * c));
                float angle = glm::pi<float>() - A;

                glm::quat knee_rot = glm::angleAxis(angle, glm::vec3(1, 0, 0));

                glm::mat4 rot_mat(knee_rot);
                knee_bone.LocalTransform[0] = rot_mat[0];
                knee_bone.LocalTransform[1] = rot_mat[1];
                knee_bone.LocalTransform[2] = rot_mat[2];

                knee_bone.GlobalTransform = knee_bone.parent->GlobalTransform * knee_bone.LocalTransform;
                target_bone.GlobalTransform = target_bone.parent->GlobalTransform * target_bone.LocalTransform;

                target_pos = target_bone.GlobalTransform[3];
            }

            OneBoneIK(leg_bone, target_pos, ik_pos);
        }
    }

    // append parent
    for (Bone *ptr : append_bones) {
        Bone &bone = *ptr;
        bone.GlobalTransform[0] = bone.append_parent->GlobalTransform[0];
        bone.GlobalTransform[1] = bone.append_parent->GlobalTransform[1];
        bone.GlobalTransform[2] = bone.append_parent->GlobalTransform[2];
    }

    // FinalTransform
    {
        for (int i = 0; i < bones.size(); i++) {
            Bone &bone = bones[i];
            FinalTransform[i] = bone.GlobalTransform * bone.InverseBindPose;
        }
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferData(GL_UNIFORM_BUFFER, FinalTransform.size() * sizeof(glm::mat4), FinalTransform.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
    }

    // morph
    {
        std::fill(morph_pos.begin(), morph_pos.end(), glm::vec3(0));
        for (Morph &morph : morphs) {
            if (anim->morph_map.count(morph.name) > 0) {
                std::vector<VMDAnimation::MorphFrame> &morph_frames = anim->morph_map[morph.name];

                auto it = std::find_if(morph_frames.begin(), morph_frames.end(), [anim_frame](VMDAnimation::MorphFrame &mf) { return mf.frame > anim_frame; });

                float weight;
                if (it == morph_frames.begin()) {
                    VMDAnimation::MorphFrame &mf = *it;
                    weight = mf.weight;
                } else if (it == morph_frames.end()) {
                    VMDAnimation::MorphFrame &mf = *std::prev(it);
                    weight = mf.weight;
                } else {
                    VMDAnimation::MorphFrame &mf0 = *std::prev(it);
                    VMDAnimation::MorphFrame &mf1 = *it;
                    float t = (anim_frame - mf0.frame) / (mf1.frame - mf0.frame);
                    weight = glm::mix(mf0.weight, mf1.weight, t);
                }

                ProcessMorph(morph, weight);
            }
        }
        glBindBuffer(GL_ARRAY_BUFFER, morph_vbo);
        glBufferData(GL_ARRAY_BUFFER, morph_pos.size() * sizeof(glm::vec3), morph_pos.data(), GL_DYNAMIC_DRAW);
    }

    // draw
    glBindVertexArray(vao);
    for (Material &material : materials) {
        if (material.texture_index != -1)
            glBindTexture(GL_TEXTURE_2D, textures[material.texture_index]);
        glDrawElements(GL_TRIANGLES, material.index_count, GL_UNSIGNED_INT, (void *) (material.index_offset * sizeof(int)));
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glBindVertexArray(0);
}

void MMDModel::ProcessMorph(Morph &morph, float weight) {
    switch (morph.type) {
        case 0: // group
            for (GroupMorph &gm : morph.group_morphs) {
                ProcessMorph(morphs[gm.index], gm.weight * weight);
            }
            break;
        case 1: // vertex
            for (VertexMorph &vm : morph.vertex_morphs) {
                morph_pos[vm.index] += vm.offset * weight;
            }
            break;
    }
}

void MMDModel::UpdateGlobalTransform(Bone *bone) {
    bone->GlobalTransform = bone->parent->GlobalTransform * bone->LocalTransform;
    for (Bone *child : bone->children)
        UpdateGlobalTransform(child);
}

void MMDModel::OneBoneIK(Bone &link_bone, glm::vec3 target_pos, glm::vec3 ik_pos) {
    glm::vec3 link_pos = link_bone.GlobalTransform[3];

    glm::vec3 from = target_pos - link_pos;
    glm::vec3 to = ik_pos - link_pos;

    from = glm::normalize(from);
    to = glm::normalize(to);

    float dot = glm::dot(from, to);
    float angle = glm::acos(dot);

    glm::vec3 axis = glm::cross(from, to);
    axis = glm::normalize(axis);

    glm::quat rot = glm::angleAxis(angle, axis);

    glm::quat global_rot(link_bone.GlobalTransform);
    global_rot = rot * global_rot;

    glm::mat4 rot_mat(global_rot);
    link_bone.GlobalTransform[0] = rot_mat[0];
    link_bone.GlobalTransform[1] = rot_mat[1];
    link_bone.GlobalTransform[2] = rot_mat[2];

    for (Bone *child : link_bone.children) {
        UpdateGlobalTransform(child);
    }
}

GLuint LoadShader() {
    const char *vertex_shader_source = R"(
        #version 460
        layout(location = 0) in vec3 pos;
        layout(location = 1) in vec2 uv;
        layout(location = 2) in ivec4 bones;
        layout(location = 3) in vec4 weights;
        layout(location = 4) in vec3 morph_pos;
        out vec2 fuv;
        uniform mat4 world;
        uniform mat4 view;
        uniform mat4 proj;
        uniform z{
            mat4 FinalTransform[1];
        };
        void main(){
        
            mat4 skinned = FinalTransform[bones[0]] * weights[0];
            skinned += FinalTransform[bones[1]] * weights[1];
            skinned += FinalTransform[bones[2]] * weights[2];
            skinned += FinalTransform[bones[3]] * weights[3];
        
            gl_Position = proj * view * world * skinned * vec4(pos + morph_pos, 1);
            fuv = uv;
        }
    )";
    const char *fragment_shader_source = R"(
        #version 460
        out vec4 color;
        in vec2 fuv;
        uniform sampler2D tex;
        void main(){
            color = texture(tex,fuv);
        }
    )";
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
    glCompileShader(vertex_shader);
    int length;
    glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &length);
    if (length > 0) {
        char *log = new char[length];
        glGetShaderInfoLog(vertex_shader, length, nullptr, log);
        std::cout << log << std::endl;
        throw 0;
    }
    glShaderSource(fragment_shader, 1, &fragment_shader_source, nullptr);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &length);
    if (length > 0) {
        char *log = new char[length];
        glGetShaderInfoLog(fragment_shader, length, nullptr, log);
        std::cout << log << std::endl;
        throw 0;
    }
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    if (length > 0) {
        char *log = new char[length];
        glGetProgramInfoLog(program, length, nullptr, log);
        std::cout << log << std::endl;
        throw 0;
    }
    return program;
}
struct VMDAnimation {

    struct BoneFrame {
        int frame;
        glm::vec3 trans;
        glm::quat rot;
    };
    struct MorphFrame {
        int frame;
        float weight;
    };

    std::map<std::wstring, std::vector<BoneFrame>> bone_map;
    std::map<std::wstring, std::vector<MorphFrame>> morph_map;

    int max_frame = 0;

    VMDAnimation(const fs::path &path);
};

struct MMDModel {

    struct Material {
        int texture_index;
        int index_count;
        int index_offset;
    };
    struct Bone {
        std::wstring name;
        glm::vec3 pos;
        Bone *parent;
        std::vector<Bone *> children;
        Bone *target;
        std::vector<Bone *> links;
        Bone *append_parent;
        glm::mat4 GlobalTransform;
        glm::mat4 LocalTransform; // 持たせる必要ないかも
        glm::mat4 InverseBindPose;
        glm::vec3 ParentOffset;
    };
    struct VertexMorph {
        int index;
        glm::vec3 offset;
    };
    struct GroupMorph {
        int index;
        float weight;
    };
    struct Morph {
        std::wstring name;
        char type; // 0:グループ, 1:頂点, 2:ボーン, 3:UV, 4:追加UV1, 5:追加UV2, 6:追加UV3, 7:追加UV4, 8:材質
        std::vector<VertexMorph> vertex_morphs;
        std::vector<GroupMorph> group_morphs;
    };

    GLuint vao, vbo, ibo, ubo, morph_vbo;
    std::vector<GLuint> textures;
    std::vector<Material> materials;
    std::vector<Bone> bones;
    std::vector<glm::mat4> FinalTransform;
    std::vector<Bone *> ik_bones;
    std::vector<Bone *> append_bones;
    std::vector<Morph> morphs;
    std::vector<glm::vec3> morph_pos;

    MMDModel(const fs::path &path);
    void Draw(VMDAnimation *anim);

  private:
    void ProcessMorph(Morph &morph, float weight);
    void UpdateGlobalTransform(Bone *bone);
    void OneBoneIK(Bone &link_bone, glm::vec3 target_pos, glm::vec3 ik_pos);
};

GLuint LoadShader();
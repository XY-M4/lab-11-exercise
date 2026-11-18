#ifndef SPRITE_MANAGER_H
#define SPRITE_MANAGER_H
#include <cstddef>
#include "basic_transform.h"
#include "texture_atlas.h"
#include "vector_util.h"
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

#define GL_SILENCE_DEPRECATION  // silences macOS OpenGL deprecation warnings

#define glCheck(expr)                                                                              \
    expr;                                                                                          \
    if (int e = glGetError()) {                                                                    \
        std::cerr << "OpenGL error: " << e << " in file " __FILE__ << ", line number " << __LINE__ \
                  << '\n';                                                                         \
        abort();                                                                                   \
    }

using SpriteID = size_t;

class SpriteManager {
public:
    SpriteManager(const sf::Vector2u& atlasSize, unsigned padding)
        : mVertices(sf::PrimitiveType::Triangles),
        mNumSprites{0},
        mAtlas(atlasSize, padding),
        mShader(std::filesystem::path{"main.vert"},
                std::filesystem::path{"main.frag"}) {

        GLuint program = mShader.getNativeHandle();

        // Get vertex attribute locations
        mPositionAttribLocation =
            glGetAttribLocation(program, "position");

        mTexCoordsAttribLocation =
            glGetAttribLocation(program, "texCoords");

        mSpriteNumberAttribLocation =
            glGetAttribLocation(program, "spriteNumber");

        // Get uniform locations
        mTextureUniformLocation =
            glGetUniformLocation(program, "texture");

        mTransformsUniformLocation =
            glGetUniformLocation(program, "transforms");

        mNumTransformsUniformLocation =
            glGetUniformLocation(program, "numTransforms");

        // Generate GPU buffers
        glGenBuffers(1, &mVerticesVBO);
        glGenBuffers(1, &mSpriteNumberVBO);

        // Generate transform texture
        glGenTextures(1, &mTransformsTexture);
    }

    ~SpriteManager() {
        glDeleteBuffers(1, &mVerticesVBO);
        glDeleteBuffers(1, &mSpriteNumberVBO);
        glDeleteTextures(1, &mTransformsTexture);
    }

    // DONE
    TextureID registerTexture(const std::string& filepath) {
        return mAtlas.registerTexture(filepath);
    }

    // DONE
    SpriteID createSprite(TextureID textureID) {
        SpriteID id = mNumSprites++;

    // Allocate 6 vertices for two triangles
    mVertices.resize(mNumSprites * 6);

    // Create default transform
    mTransforms.emplace_back();
    // Record the sprite ID for each of its 6 vertices
    mSpriteNumbers.insert(
        mSpriteNumbers.end(),
        6,
        static_cast<float>(id)
    );

        // Set vertex positions and UV coordinates
        setTexture(id, textureID);

        return id;
    }

    // DONE
    void setTexture(SpriteID sprite, TextureID newTexture) {
        assert(sprite < mNumSprites);

        sf::Vector2u size = mAtlas.getSizePixels(newTexture);
        sf::FloatRect uv = mAtlas.getBoundsUV(newTexture);

        float w = static_cast<float>(size.x);
        float h = static_cast<float>(size.y);

        float u0 = uv.position.x;
        float v0 = uv.position.y;
        float u1 = u0 + uv.size.x;
        float v1 = v0 + uv.size.y;

        size_t start = sprite * 6;

        // First triangle
        mVertices[start + 0] = {{0.f, 0.f}, sf::Color::White, {u0, v0}};
        mVertices[start + 1] = {{w, 0.f}, sf::Color::White, {u1, v0}};
        mVertices[start + 2] = {{w, h}, sf::Color::White, {u1, v1}};

        // Second triangle
        mVertices[start + 3] = {{0.f, 0.f}, sf::Color::White, {u0, v0}};
        mVertices[start + 4] = {{w, h}, sf::Color::White, {u1, v1}};
        mVertices[start + 5] = {{0.f, h}, sf::Color::White, {u0, v1}};
    }

    // TODO
    void draw(sf::RenderTarget& target, sf::RenderStates states) const {
        // Copy all sprite vertices
        sf::VertexArray vertices = mVertices;

        // Apply each sprite's transform
        for (size_t sprite = 0; sprite < mNumSprites; ++sprite) {
            const BasicTransform& transform = mTransforms[sprite];

            for (size_t i = 0; i < 6; ++i) {
                sf::Vector2f& pos = vertices[sprite * 6 + i].position;

                pos = {
                    pos.x * transform.scale.x + transform.translation.x,
                    pos.y * transform.scale.y + transform.translation.y
                };
            }
        }

    // Configure atlas and shader
    states.texture = &mAtlas.getTexture();
    states.shader = &mShader;
    states.coordinateType = sf::CoordinateType::Normalized;

    mShader.setUniform("texture", sf::Shader::CurrentTexture);

    // Draw all sprites in one call
    target.draw(vertices, states);
}   

    // TODO
    void drawGL() {
        if (mNumSprites == 0) return;

    glUseProgram(mShader.getNativeHandle());

    // Upload vertices
    glBindBuffer(GL_ARRAY_BUFFER, mVerticesVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        mVertices.getVertexCount() * sizeof(sf::Vertex),
        &mVertices[0],
        GL_STREAM_DRAW
    );

    glVertexAttribPointer(
        mPositionAttribLocation, 2, GL_FLOAT, GL_FALSE,
        sizeof(sf::Vertex),
        reinterpret_cast<const void*>(offsetof(sf::Vertex, position))
    );
    glEnableVertexAttribArray(mPositionAttribLocation);

    glVertexAttribPointer(
        mTexCoordsAttribLocation, 2, GL_FLOAT, GL_FALSE,
        sizeof(sf::Vertex),
        reinterpret_cast<const void*>(offsetof(sf::Vertex, texCoords))
    );
    glEnableVertexAttribArray(mTexCoordsAttribLocation);

    // Upload sprite numbers
    glBindBuffer(GL_ARRAY_BUFFER, mSpriteNumberVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        mSpriteNumbers.size() * sizeof(float),
        mSpriteNumbers.data(),
        GL_STREAM_DRAW
    );

    glVertexAttribPointer(
        mSpriteNumberAttribLocation, 1, GL_FLOAT,
        GL_FALSE, sizeof(float), nullptr
    );
    glEnableVertexAttribArray(mSpriteNumberAttribLocation);

    // Bind atlas texture to texture unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(
        GL_TEXTURE_2D,
        mAtlas.getTexture().getNativeHandle()
    );

    glUniform1i(mTextureUniformLocation, 0);

    // Pack transforms as RGBA floats
    std::vector<float> transformData;
    transformData.reserve(mTransforms.size() * 4);

    for (const BasicTransform& t : mTransforms) {
        transformData.push_back(t.scale.x);
        transformData.push_back(t.scale.y);
        transformData.push_back(t.translation.x);
        transformData.push_back(t.translation.y);
    }

    // Upload transforms to texture unit 1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, mTransformsTexture);

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

    glTexImage1D(
        GL_TEXTURE_1D,
        0,
        GL_RGBA32F_ARB,
        static_cast<GLsizei>(mTransforms.size()),
        0,
        GL_RGBA,
        GL_FLOAT,
        transformData.data()
    );

    glUniform1i(mTransformsUniformLocation, 1);
    glUniform1i(
        mNumTransformsUniformLocation,
        static_cast<GLint>(mTransforms.size())
    );

    // Enable transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA); //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Draw all sprites in one call
    glDrawArrays(
        GL_TRIANGLES,
        0,
        static_cast<GLsizei>(mVertices.getVertexCount())
    );

    glDisableVertexAttribArray(mPositionAttribLocation);
    glDisableVertexAttribArray(mTexCoordsAttribLocation);
    glDisableVertexAttribArray(mSpriteNumberAttribLocation);
}

    // TODO
    BasicTransform& getTransform(SpriteID id) {
        assert(id < mTransforms.size());
        return mTransforms[id];
    }

    // TODO
    const TextureAtlas& getAtlas() const {
        return mAtlas;
    }

private:
    sf::VertexArray mVertices;
    std::vector<BasicTransform> mTransforms;
    size_t mNumSprites;
    TextureAtlas mAtlas;
    mutable sf::Shader mShader;

    // Use the below only in Batched rendering (part 2):
    std::vector<float> mSpriteNumbers;

    GLint mTextureUniformLocation;
    GLint mTransformsUniformLocation;
    GLint mNumTransformsUniformLocation;

    GLint mPositionAttribLocation;
    GLint mTexCoordsAttribLocation;
    GLint mSpriteNumberAttribLocation;

    GLuint mVerticesVBO;
    GLuint mSpriteNumberVBO;
    GLuint mTransformsTexture;
};

#endif  // SPRITE_MANAGER_H

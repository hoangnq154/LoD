#include "terrain.h"

using namespace oglwrap;

Terrain::Terrain(Skybox& skybox)
    : vs("terrain.vert")
    , fs("terrain.frag")
    , projectionMatrix(prog, "ProjectionMatrix")
    , cameraMatrix(prog, "CameraMatrix")
    , sunData(prog, "SunData")
    , scales(prog, "Scales")
    , offset(prog, "Offset")
    , mipmapLevel(prog, "MipmapLevel")
    , heightMap(prog, "HeightMap")
    , normalMap(prog, "NormalMap")
    , colorMap(prog, "ColorMap")
    , mesh("terrain/mideu.rtd",
           "terrain/mideu.rtc")
    , skybox(skybox) {

    prog << vs << fs << skybox.sky_fs;
    prog.Link().Use();

    heightMap.Set(0);
    colorMap.Set(1);
}

void Terrain::Reshape(const glm::mat4& projMat) {
    projectionMatrix = projMat;
}

void Terrain::Render(float time, const glm::mat4& camMat, const glm::vec3& camPos) {
    prog.Use();
    cameraMatrix.Set(camMat);
    sunData.Set(skybox.getSunData(time));
    mesh.Render(camPos, offset, scales, mipmapLevel);
}

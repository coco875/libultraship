#pragma once

#include "resource/Resource.h"
#include "resource/type/Shader.h"
#include "resource/ResourceFactoryBinary.h"

namespace Ship {
class ResourceFactoryBinaryShaderV0 : public ResourceFactoryBinary {
  public:
    std::shared_ptr<IResource> ReadResource(std::shared_ptr<File> file, std::shared_ptr<Ship::ResourceInitData> initData) override;
};
}; // namespace Ship

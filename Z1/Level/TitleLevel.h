#pragma once
#include <Level/Level.h>
#include <memory>
#include <filesystem>

namespace Craft
{
    class Sprite;
}

class TitleLevel : public Craft::Level
{
    using FilePath = std::filesystem::path;

public:
    TitleLevel();
    ~TitleLevel() override = default;

    void OnInitialized() override;
    void BeginPlay() override;
    void Tick(float deltaTime) override;
    void Draw() override;
    void EndPlay() override;

private:
    std::shared_ptr<const Craft::Sprite> LoadTitleSprite(const FilePath& path);

private:
    std::shared_ptr<const Craft::Sprite> _titleSprite;
    bool _bgmStarted = false;
};


#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/utils/web.hpp>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

using namespace geode::prelude;

namespace {
    // Vanilla Geometry Dash decorative object IDs.
    constexpr int kGlowObjectID = 503;
    constexpr int kShineObjectID = 1021;

    constexpr size_t kMaxBasicTargets = 300;
    constexpr size_t kMaxAIAnchors = 80;
    constexpr size_t kMaxAIPlacements = 160;
    constexpr float kDuplicateRadius = 10.f;
    constexpr float kDuplicateRadiusSquared = kDuplicateRadius * kDuplicateRadius;

    bool hasDecorationNear(std::vector<CCPoint> const& positions, CCPoint target) {
        for (auto const& point : positions) {
            float dx = point.x - target.x;
            float dy = point.y - target.y;
            if (dx * dx + dy * dy <= kDuplicateRadiusSquared) {
                return true;
            }
        }
        return false;
    }

    std::string cleanModelJSON(std::string text) {
        // Providers sometimes wrap JSON in Markdown fences despite being asked not to.
        auto first = text.find('{');
        auto last = text.rfind('}');
        if (first != std::string::npos && last != std::string::npos && last >= first) {
            return text.substr(first, last - first + 1);
        }
        return text;
    }

    void showAutoDecoError(std::string const& message) {
        FLAlertLayer::create("Auto Deco", message.c_str(), "OK")->show();
    }
}

class $modify(AutoDecoEditorUI, EditorUI) {
    struct Fields : TextInputDelegate {
        CCTextInputNode* promptInput = nullptr;
        async::TaskHolder<web::WebResponse> aiRequest;
        bool aiRunning = false;

        // The input still works as a text field; this delegate is present so the
        // iOS text-input node can report edits normally.
        void textChanged(CCTextInputNode*) override {}
    };

    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) {
            return false;
        }

        auto menu = CCMenu::create();
        if (!menu) return true;
        menu->setID("auto-deco-menu"_spr);
        menu->setPosition(CCPointZero);

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        // Keep the original one-tap local decorator as a fallback.
        auto basicButton = this->getButton(
            "AUTO",
            48,
            menu_selector(AutoDecoEditorUI::onAutoDeco),
            menu
        );
        if (basicButton) {
            basicButton->setID("auto-deco-basic-button"_spr);
            basicButton->setPosition({winSize.width - 62.f, winSize.height - 66.f});
        }

        auto aiButton = this->getButton(
            "AI",
            48,
            menu_selector(AutoDecoEditorUI::onAIDeco),
            menu
        );
        if (aiButton) {
            aiButton->setID("auto-deco-ai-button"_spr);
            aiButton->setPosition({winSize.width - 52.f, winSize.height - 30.f});
        }

        this->addChild(menu, 10000);

        // Prompt field next to the AI button. Tap it, type a request, then tap AI.
        auto promptInput = CCTextInputNode::create(
            300.f,
            24.f,
            "Describe the decoration...",
            "bigFont.fnt"
        );
        if (promptInput) {
            promptInput->setID("auto-deco-prompt-input"_spr);
            promptInput->setDelegate(m_fields.self());
            promptInput->setScale(0.68f);
            promptInput->setPosition({winSize.width - 205.f, winSize.height - 30.f});
            this->addChild(promptInput, 10001);
            m_fields->promptInput = promptInput;
        }

        return true;
    }

    std::vector<CCPoint> getSolidPositions() {
        std::vector<CCPoint> solidPositions;
        if (!m_editorLayer) return solidPositions;

        auto objects = m_editorLayer->getAllObjects();
        if (!objects) return solidPositions;
        solidPositions.reserve(objects->count());

        for (unsigned int i = 0; i < objects->count(); ++i) {
            auto object = static_cast<GameObject*>(objects->objectAtIndex(i));
            if (!object) continue;
            if (object->m_objectType == GameObjectType::Solid &&
                !object->m_isDecoration &&
                !object->m_isDecoration2 &&
                !object->m_isTrigger) {
                solidPositions.push_back(object->getPosition());
            }
        }

        // Keep the AI's anchor list in level order even if the object array isn't.
        std::sort(solidPositions.begin(), solidPositions.end(), [](CCPoint const& a, CCPoint const& b) {
            if (std::abs(a.x - b.x) > 0.1f) return a.x < b.x;
            return a.y < b.y;
        });
        return solidPositions;
    }

    void onAutoDeco(CCObject*) {
        if (!m_editorLayer) {
            showAutoDecoError("Open a level in the editor first.");
            return;
        }

        auto objects = m_editorLayer->getAllObjects();
        if (!objects) {
            showAutoDecoError("Couldn't read the objects in this level.");
            return;
        }

        auto solidPositions = getSolidPositions();
        if (solidPositions.empty()) {
            showAutoDecoError("I couldn't find any solid blocks to decorate in this level.");
            return;
        }

        std::vector<CCPoint> glowPositions;
        std::vector<CCPoint> shinePositions;
        for (unsigned int i = 0; i < objects->count(); ++i) {
            auto object = static_cast<GameObject*>(objects->objectAtIndex(i));
            if (!object) continue;
            if (object->m_objectID == kGlowObjectID) glowPositions.push_back(object->getPosition());
            if (object->m_objectID == kShineObjectID) shinePositions.push_back(object->getPosition());
        }

        size_t targetCount = std::min(solidPositions.size(), kMaxBasicTargets);
        size_t glowsAdded = 0;
        size_t shinesAdded = 0;

        for (size_t sample = 0; sample < targetCount; ++sample) {
            size_t index = sample * solidPositions.size() / targetCount;
            auto blockPos = solidPositions[index];

            if (!hasDecorationNear(glowPositions, blockPos)) {
                this->createObject(kGlowObjectID, blockPos);
                glowPositions.push_back(blockPos);
                ++glowsAdded;
            }

            if (sample % 5 == 2) {
                CCPoint shinePos = {blockPos.x + 13.f, blockPos.y + 18.f};
                if (!hasDecorationNear(shinePositions, shinePos)) {
                    this->createObject(kShineObjectID, shinePos);
                    shinePositions.push_back(shinePos);
                    ++shinesAdded;
                }
            }
        }

        if (glowsAdded == 0 && shinesAdded == 0) {
            showAutoDecoError("This layout already has these Auto Deco accents nearby. Nothing new was added.");
            return;
        }

        auto message = std::string("Added ") + std::to_string(glowsAdded) +
            " glow pieces and " + std::to_string(shinesAdded) +
            " shine accents. Use the editor's normal Undo controls to remove placements if needed.";
        showAutoDecoError(message);
    }

    void onAIDeco(CCObject*) {
        if (!m_editorLayer) {
            showAutoDecoError("Open a level in the editor first.");
            return;
        }
        if (!m_fields->promptInput) {
            showAutoDecoError("The prompt box didn't initialize. Close and reopen the editor.");
            return;
        }

        auto rawPrompt = m_fields->promptInput->getString();
        std::string prompt = rawPrompt;
        if (prompt.find_first_not_of(" \t\r\n") == std::string::npos) {
            showAutoDecoError("Type a decoration prompt first, such as: 'Use sparse glows below the blocks with a shine every few blocks.'");
            return;
        }

        if (m_fields->aiRunning) {
            showAutoDecoError("The AI is already working on a prompt. Wait for that request to finish first.");
            return;
        }

        auto apiKey = Mod::get()->getSettingValue<std::string>("api-key");
        auto endpoint = Mod::get()->getSettingValue<std::string>("api-endpoint");
        auto model = Mod::get()->getSettingValue<std::string>("api-model");
        if (endpoint.empty() || model.empty()) {
            showAutoDecoError("Set a valid API endpoint and model in Auto Deco's mod settings.");
            return;
        }
        if (endpoint.find("YOUR-MAC-IP") != std::string::npos) {
            showAutoDecoError(
                "Replace YOUR-MAC-IP in Auto Deco's endpoint setting with your Mac's Wi-Fi IP address. "
                "See the included README for the local Ollama setup."
            );
            return;
        }
        if (apiKey.empty() && endpoint.rfind("https://", 0) == 0) {
            showAutoDecoError(
                "This HTTPS provider may require an API key. For the no-key local setup, run Ollama on your Mac "
                "and use http://YOUR-MAC-IP:11434/v1/chat/completions in the endpoint setting."
            );
            return;
        }

        auto solids = getSolidPositions();
        if (solids.empty()) {
            showAutoDecoError("I couldn't find any solid blocks to decorate in this level.");
            return;
        }

        std::vector<CCPoint> anchors;
        auto anchorCount = std::min(solids.size(), kMaxAIAnchors);
        anchors.reserve(anchorCount);
        for (size_t i = 0; i < anchorCount; ++i) {
            size_t index = i * solids.size() / anchorCount;
            anchors.push_back(solids[index]);
        }

        std::ostringstream anchorText;
        anchorText << "User decoration request: " << prompt << "\n\n"
                   << "Available solid-block anchors (index: x, y). These are sampled across the level:\n"
                   << std::fixed << std::setprecision(1);
        for (size_t i = 0; i < anchors.size(); ++i) {
            anchorText << i << ": " << anchors[i].x << ", " << anchors[i].y << "\n";
        }

        std::string systemPrompt =
            "You are an assistant that plans decorative accents for a Geometry Dash level editor. "
            "You may ONLY place two existing decorative object kinds: 'glow' (object ID 503) and 'shine' (object ID 1021). "
            "Use the user's natural-language prompt to determine placement density, pattern, spacing, and offset. "
            "Never create gameplay objects or invent object IDs. Return ONLY valid JSON shaped like: "
            "{\"placements\":[{\"anchor\":0,\"kind\":\"glow\",\"dx\":0,\"dy\":-12}]}. "
            "Each placement must reference an anchor index from the supplied list. 'anchor' must be an integer. "
            "'kind' must be exactly 'glow' or 'shine'. 'dx' and 'dy' must be integers between -32 and 32. "
            "Return no more than 160 placements. Do not include Markdown or explanatory text. "
            "If the prompt asks for a type of decoration you cannot create with glow/shine, approximate its placement pattern using those two types.";

        auto messages = matjson::Value::array();
        messages.push(matjson::makeObject({
            {"role", "system"},
            {"content", systemPrompt}
        }));
        messages.push(matjson::makeObject({
            {"role", "user"},
            {"content", anchorText.str()}
        }));

        auto requestBody = matjson::makeObject({
            {"model", model},
            {"messages", messages},
            {"temperature", 0.25}
        });

        web::WebRequest request;
        request.header("Content-Type", "application/json");
        if (!apiKey.empty()) {
            request.header("Authorization", "Bearer " + apiKey);
        }
        request.timeout(std::chrono::seconds(60));
        request.bodyJSON(requestBody);

        m_fields->aiRunning = true;
        log::info("Auto Deco AI: sent the decoration prompt to the configured provider.");
        m_fields->aiRequest.spawn(
            request.post(endpoint),
            [this, anchors = std::move(anchors)](web::WebResponse response) {
                m_fields->aiRunning = false;
                if (!response.ok()) {
                    auto body = response.string().unwrapOr("No error details returned.");
                    if (body.size() > 240) body = body.substr(0, 240) + "...";
                    auto message = "The AI service returned HTTP " + std::to_string(response.code()) + ":\n" + body;
                    showAutoDecoError(message);
                    return;
                }

                auto responseJsonResult = response.json();
                if (!responseJsonResult) {
                    showAutoDecoError("The AI service returned invalid JSON. Check your endpoint and model settings.");
                    return;
                }
                auto responseJson = responseJsonResult.unwrap();
                auto content = responseJson["choices"][0]["message"]["content"].asString().unwrapOr("");
                if (content.empty()) {
                    showAutoDecoError("The AI returned no plan. Check the API response, model, and API-key settings.");
                    return;
                }

                auto planResult = matjson::Value::parse(cleanModelJSON(content));
                if (!planResult) {
                    showAutoDecoError("The AI plan wasn't valid JSON. Try a shorter, more direct prompt.");
                    return;
                }
                auto plan = planResult.unwrap();
                auto placements = plan["placements"];
                if (!placements.isArray()) {
                    showAutoDecoError("The AI plan didn't include a placements array. Try your prompt again.");
                    return;
                }

                // Re-scan current decorations at apply time to avoid duplicates if
                // the user edited the level while the request was running.
                std::vector<CCPoint> glowPositions;
                std::vector<CCPoint> shinePositions;
                auto currentObjects = m_editorLayer ? m_editorLayer->getAllObjects() : nullptr;
                if (currentObjects) {
                    for (unsigned int i = 0; i < currentObjects->count(); ++i) {
                        auto object = static_cast<GameObject*>(currentObjects->objectAtIndex(i));
                        if (!object) continue;
                        if (object->m_objectID == kGlowObjectID) glowPositions.push_back(object->getPosition());
                        if (object->m_objectID == kShineObjectID) shinePositions.push_back(object->getPosition());
                    }
                }

                size_t addedGlows = 0;
                size_t addedShines = 0;
                size_t accepted = 0;
                for (auto const& placement : placements) {
                    if (accepted >= kMaxAIPlacements) break;

                    int anchor = placement["anchor"].asInt().unwrapOr(-1);
                    std::string kind = placement["kind"].asString().unwrapOr("");
                    int dx = placement["dx"].asInt().unwrapOr(0);
                    int dy = placement["dy"].asInt().unwrapOr(0);
                    if (anchor < 0 || static_cast<size_t>(anchor) >= anchors.size()) continue;
                    if (kind != "glow" && kind != "shine") continue;

                    // Clamp model-provided offsets even when the model ignores instructions.
                    dx = std::clamp(dx, -32, 32);
                    dy = std::clamp(dy, -32, 32);
                    CCPoint position = {
                        anchors[static_cast<size_t>(anchor)].x + static_cast<float>(dx),
                        anchors[static_cast<size_t>(anchor)].y + static_cast<float>(dy)
                    };

                    auto& sameKindPositions = kind == "glow" ? glowPositions : shinePositions;
                    if (hasDecorationNear(sameKindPositions, position)) continue;

                    this->createObject(kind == "glow" ? kGlowObjectID : kShineObjectID, position);
                    sameKindPositions.push_back(position);
                    if (kind == "glow") ++addedGlows;
                    else ++addedShines;
                    ++accepted;
                }

                if (addedGlows == 0 && addedShines == 0) {
                    showAutoDecoError("The AI plan didn't add any new decorations. Try a different prompt or remove nearby duplicate accents.");
                    return;
                }

                auto summary = std::string("AI placed ") + std::to_string(addedGlows) +
                    " glows and " + std::to_string(addedShines) +
                    " shines based on your prompt. Use the editor's Undo controls to remove the placements.";
                showAutoDecoError(summary);
            }
        );
    }
};

#include "Renderer.h"
#include "IRenderBackend.h"
#include "RendererBackendFactory.h"

namespace Anjean
{
    Renderer::Renderer(Window& window, const RendererConfig& config)
        : m_backend(CreateRendererBackend(window, config))
    {
    }

    Renderer::~Renderer() = default;

    void Renderer::beginFrame(const Color& clearColor)
    {
        m_backend->beginFrame(clearColor);
    }

    void Renderer::endFrame()
    {
        m_backend->endFrame();
    }
    
    bool Renderer::renderRect(Renderer *renderer, const Rect *rect)
    {
        m_backend->endFrame();
        return true;
    }

    void Renderer::drawTestTriangle()
    {
        m_backend->drawTestTriangle();
    }
}
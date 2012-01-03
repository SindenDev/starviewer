#include "magnifyingglasstool.h"

#include "q2dviewer.h"
#include "logging.h"
#include "coresettings.h"

// vtk
#include <vtkCommand.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <vtkImageActor.h>

namespace udg {

MagnifyingGlassTool::MagnifyingGlassTool(QViewer *viewer, QObject *parent)
 : Tool(viewer,parent)
{
    m_toolName = "MagnifyingGlassTool";
    m_magnifyingWindowShown = false;
    
    m_2DViewer = qobject_cast<Q2DViewer*>(viewer);
    if (!m_2DViewer)
    {
        DEBUG_LOG(QString("El casting no ha funcionat!!! �s possible que viewer no sigui un Q2DViewer!!!-> ")+ viewer->metaObject()->className());
    }

    m_magnifiedRenderer = vtkRenderer::New();
}

MagnifyingGlassTool::~MagnifyingGlassTool()
{
    hideMagnifiedRenderer();
    m_magnifiedRenderer->Delete();
}

void MagnifyingGlassTool::handleEvent(unsigned long eventID)
{
    switch (eventID)
    {
        case vtkCommand::LeftButtonPressEvent:
            enableConnections();
            updateMagnifiedImage();
            break;

        case vtkCommand::MouseMoveEvent:
            if (m_magnifyingWindowShown)
            {
                updateMagnifiedImage();
            }
            break;

        case vtkCommand::LeftButtonReleaseEvent:
        case vtkCommand::LeaveEvent:
            enableConnections(false);
            hideMagnifiedRenderer();
            break;

        default:
            break;
    }
}

void MagnifyingGlassTool::updateMagnifiedViewportPosition()
{
    // Movem la finestra per que acompanyi el cursor
    QPoint eventPosition = m_2DViewer->getEventPosition();
    QSize size = m_2DViewer->getRenderWindowSize();

    double magnifyingWindowSize = 150.0;

    double xMin = eventPosition.x() / (double)size.width() - magnifyingWindowSize / size.width();
    double xMax = eventPosition.x() / (double)size.width() + magnifyingWindowSize / size.width();
    double yMin = eventPosition.y() / (double)size.height() - magnifyingWindowSize / size.height();
    double yMax = eventPosition.y() / (double)size.height() + magnifyingWindowSize / size.height();

    if (xMin < 0)
    {
        xMin = 0;
        xMax = (magnifyingWindowSize / size.width()) * 2;
    }
    if (yMin < 0)
    {
        yMin = 0;
        yMax = (magnifyingWindowSize / size.height()) * 2;
    }
    if (xMax > 1)
    {
        xMin = 1 - ((magnifyingWindowSize / size.width()) * 2);
        xMax = 1;
    }
    if (yMax > 1)
    {
        yMin = 1 - ((magnifyingWindowSize / size.height()) * 2);
        yMax = 1;
    }

    m_magnifiedRenderer->SetViewport(xMin, yMin, xMax, yMax);
    m_2DViewer->render();
}

void MagnifyingGlassTool::hideMagnifiedRenderer()
{
    if (m_magnifyingWindowShown)
    {
        m_2DViewer->setCursor(QCursor(Qt::ArrowCursor));
        m_2DViewer->getRenderWindow()->RemoveRenderer(m_magnifiedRenderer);
        m_2DViewer->render();
        m_magnifyingWindowShown = false;
    }
}

double MagnifyingGlassTool::getZoomFactor()
{
    Settings settings;
    double factor = settings.getValue(CoreSettings::MagnifyingGlassZoomFactor).toDouble();
    if (factor == 0.0)
    {
        // En cas que el setting no tingui un valor v�lid, li assignem un valor per defecte de 4.0
        factor = 4.0;
        settings.setValue(CoreSettings::MagnifyingGlassZoomFactor, "4");
    }

    return factor;
}

void MagnifyingGlassTool::updateMagnifiedRenderer()
{
    // TODO Nom�s s'afegeix una sola vegada si ja existeix??? Comprovar!
    m_2DViewer->getRenderWindow()->AddRenderer(m_magnifiedRenderer);
    
    updateCamera();

    // TODO Nom�s s'afegeix una sola vegada si ja existeix??? Comprovar!
    m_magnifiedRenderer->AddViewProp(m_2DViewer->getVtkImageActor());
    m_magnifyingWindowShown = true;
}

void MagnifyingGlassTool::updateMagnifiedImage()
{
    double xyz[3];
    if (m_2DViewer->getCurrentCursorImageCoordinate(xyz))
    {
        m_2DViewer->setCursor(QCursor(Qt::BlankCursor));
        if (!m_magnifyingWindowShown)
        {
            updateMagnifiedRenderer();
        }
        // Actualitzem la posici� de la imatge
        m_magnifiedCamera->SetFocalPoint(xyz);
        m_magnifiedRenderer->ResetCameraClippingRange();
        updateMagnifiedViewportPosition();
        m_2DViewer->render();
    }
    else
    {
        if (m_magnifyingWindowShown)
        {
            hideMagnifiedRenderer();
        }
    }
}

void MagnifyingGlassTool::updateCamera()
{
    if (!m_magnifiedCamera)
    {
        m_magnifiedCamera = vtkSmartPointer<vtkCamera>::New();
    }
    m_magnifiedCamera->DeepCopy(m_2DViewer->getRenderer()->GetActiveCamera());
    m_magnifiedRenderer->SetActiveCamera(m_magnifiedCamera);

    // Codi extret de QViewer::zoom(). TODO Fer refactoring
    if (m_magnifiedCamera->GetParallelProjection())
    {
        m_magnifiedCamera->SetParallelScale(m_magnifiedCamera->GetParallelScale() / getZoomFactor());
    }
    else
    {
        m_magnifiedCamera->Dolly(getZoomFactor());
        //if (vtkInteractorStyle::SafeDownCast(this->getInteractor()->GetInteractorStyle())->GetAutoAdjustCameraClippingRange())
        //{
            // TODO en principi sempre ens interessar� fer aix�? ens podriem enstalviar l'if??
            m_magnifiedRenderer->ResetCameraClippingRange();
        //}
    }
}

void MagnifyingGlassTool::update()
{
    updateCamera();
    updateMagnifiedImage();
}

void MagnifyingGlassTool::enableConnections(bool enable)
{
    if (enable)
    {
        connect(m_2DViewer, SIGNAL(volumeChanged(Volume*)), SLOT(hideMagnifiedRenderer()));
        connect(m_2DViewer, SIGNAL(sliceChanged(int)), SLOT(updateMagnifiedImage()));
        connect(m_2DViewer, SIGNAL(cameraChanged()), SLOT(update()));
    }
    else
    {
        disconnect(m_2DViewer, SIGNAL(volumeChanged(Volume*)), this, SLOT(hideMagnifiedRenderer()));
        disconnect(m_2DViewer, SIGNAL(sliceChanged(int)), this, SLOT(updateMagnifiedImage()));
        disconnect(m_2DViewer, SIGNAL(cameraChanged()), this, SLOT(update()));
    }
}

}

/*************************************************************************************
  Copyright (C) 2014 Laboratori de Gràfics i Imatge, Universitat de Girona &
  Institut de Diagnòstic per la Imatge.
  Girona 2014. All rights reserved.
  http://starviewer.udg.edu

  This file is part of the Starviewer (Medical Imaging Software) open source project.
  It is subject to the license terms in the LICENSE file found in the top-level
  directory of this distribution and at http://starviewer.udg.edu/license. No part of
  the Starviewer (Medical Imaging Software) open source project, including this file,
  may be copied, modified, propagated, or distributed except according to the
  terms contained in the LICENSE file.
 *************************************************************************************/

#include "q2dviewerannotationhandler.h"

#include "q2dviewer.h"
#include "mathtools.h"
#include "mammographyimagehelper.h"
#include "image.h"
#include "study.h"
#include "patient.h"
#include "volume.h"
#include "volumehelper.h"
#include "logging.h"

#include <vtkCornerAnnotation.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkRenderer.h>

namespace udg {

Q2DViewerAnnotationHandler::Q2DViewerAnnotationHandler(Q2DViewer *viewer)
{
    m_2DViewer = viewer;

    createAnnotations();
    addActors();
}

Q2DViewerAnnotationHandler::~Q2DViewerAnnotationHandler()
{
    for (int i = 0; i < 4; ++i)
    {
        m_patientOrientationTextActor[i]->Delete();
    }
    m_cornerAnnotations->Delete();
}

void Q2DViewerAnnotationHandler::enableAnnotation(AnnotationFlags annotation, bool enable)
{
    if (enable)
    {
        m_enabledAnnotations = m_enabledAnnotations | annotation;
    }
    else
    {
        m_enabledAnnotations = m_enabledAnnotations & ~annotation;
    }

    refreshAnnotations();

    if (m_2DViewer->hasInput())
    {
        m_2DViewer->render();
    }
}

void Q2DViewerAnnotationHandler::removeAnnotation(AnnotationFlags annotation)
{
    enableAnnotation(annotation, false);
}

void Q2DViewerAnnotationHandler::updateAnnotationsInformation(AnnotationFlags annotation)
{
    if (!m_2DViewer->hasInput())
    {
        return;
    }

    if (annotation.testFlag(VoiLutInformationAnnotation))
    {
        updateVoiLutInformationAnnotation();
    }

    if (annotation.testFlag(SliceAnnotation))
    {
        updateSliceAnnotationInformation();
    }
}

void Q2DViewerAnnotationHandler::updatePatientAnnotationInformation()
{
    if (m_2DViewer->hasInput())
    {
        // TODO We only take the first image for the moment because we assume all belong to the same series
        Image *image = m_2DViewer->getMainInput()->getImage(0);
        Series *series = image->getParentSeries();
        Study *study = series->getParentStudy();
        Patient *patient = study->getParentPatient();

        // Permanent information
        QString seriesTime = series->getTimeAsString();
        if (seriesTime.isEmpty())
        {
            seriesTime = "--:--";
        }

        m_upperRightText = series->getInstitutionName() + "\n";
        m_upperRightText += patient->getFullName() + "\n";
        m_upperRightText += QString("%1 %2 %3").arg(study->getPatientAge()).arg(patient->getSex()).arg(patient->getID());
        m_upperRightText += "\n";
        if (!study->getAccessionNumber().isEmpty())
        {
            m_upperRightText += QObject::tr("Acc: %1").arg(study->getAccessionNumber());
        }
        m_upperRightText += "\n";
        m_upperRightText += study->getDate().toString(Qt::ISODate) + "\n";
        m_upperRightText += seriesTime;

        if (series->getModality() == "MG")
        {
            m_lowerRightText.clear();
        }
        else
        {
            m_lowerRightText = getSeriesDescriptiveLabel(series);
            
            if (m_2DViewer->getNumberOfInputs() == 2)
            {
                int balance = m_2DViewer->getFusionBalance();
                const QString &modality0 = m_2DViewer->getInput(0)->getModality();
                const QString &modality1 = m_2DViewer->getInput(1)->getModality();
                QString balanceText = QString("%1% %2 + %3% %4").arg(100 - balance).arg(modality0).arg(balance).arg(modality1);
                QString fusedLabel = getSeriesDescriptiveLabel(m_2DViewer->getInput(1)->getImage(0)->getParentSeries());

                m_lowerRightText = QObject::tr("Fusion: ") + balanceText + "\n" + m_lowerRightText + " +\n" + fusedLabel;
            }
        }

        if (m_enabledAnnotations.testFlag(PatientInformationAnnotation))
        {
            setCornerAnnotation(UpperRightCornerIndex, m_upperRightText);
            setCornerAnnotation(LowerRightCornerIndex, m_lowerRightText.trimmed());
        }
    }
    else
    {
        DEBUG_LOG("There's no input. Patient information cannot be initialized.");
    }
}

void Q2DViewerAnnotationHandler::updateSliceAnnotationInformation()
{
    Q_ASSERT(m_cornerAnnotations);
    Q_ASSERT(m_2DViewer->hasInput());
    
    Image *image = m_2DViewer->getCurrentDisplayedImage();
    
    MammographyImageHelper mammographyImageHelper;
    if (mammographyImageHelper.isStandardMammographyImage(image))
    {
        // Specific mammography annotations should be displayed
        m_enabledAnnotations = m_enabledAnnotations & ~SliceAnnotation;
        
        QString laterality = image->getImageLaterality();
        QString projection = mammographyImageHelper.getMammographyProjectionLabel(image);

        m_lowerRightText = laterality + " " + projection;
        
        setCornerAnnotation(LowerRightCornerIndex, m_lowerRightText.trimmed());
    }
    else
    {
        if (m_enabledAnnotations.testFlag(PatientInformationAnnotation))
        {
            updateLateralityAnnotationInformation();
        }
    }

    updateSliceAnnotation();
    updatePatientInformationAnnotation();
}

void Q2DViewerAnnotationHandler::updatePatientOrientationAnnotation()
{
    // Get the current image orientation
    PatientOrientation currentPatientOrientation = m_2DViewer->getCurrentDisplayedImagePatientOrientation();

    // Indices relationship: 0:Left, 1:Bottom, 2:Right, 3:Top
    m_patientOrientationText[LeftOrientationLabelIndex] = PatientOrientation::getOppositeOrientationLabel(currentPatientOrientation.getRowDirectionLabel());
    m_patientOrientationText[BottomOrientationLabelIndex] = currentPatientOrientation.getColumnDirectionLabel();
    m_patientOrientationText[RightOrientationLabelIndex] = currentPatientOrientation.getRowDirectionLabel();
    m_patientOrientationText[TopOrientationLabelIndex] = PatientOrientation::getOppositeOrientationLabel(currentPatientOrientation.getColumnDirectionLabel());

    // #1349: Hide the posterior label in mammographies so it doesn't cover the image
    MammographyImageHelper mammographyImageHelper;
    Image *image = m_2DViewer->getCurrentDisplayedImage();

    if (mammographyImageHelper.isStandardMammographyImage(image))
    {
        m_patientOrientationText[LeftOrientationLabelIndex].remove(PatientOrientation::PosteriorLabel);
        m_patientOrientationText[RightOrientationLabelIndex].remove(PatientOrientation::PosteriorLabel);
        // HACK: If the orientation has changed we may need to swap sides of annotations in mammography, and this is the easiest way to ensure it right now
        refreshAnnotations();
    }
    
    bool textActorShouldBeVisible = m_enabledAnnotations.testFlag(PatientOrientationAnnotation);

    for (int i = 0; i < 4; ++i)
    {
        if (!m_patientOrientationText[i].isEmpty())
        {
            m_patientOrientationTextActor[i]->SetInput(m_patientOrientationText[i].toUtf8().constData());
            m_patientOrientationTextActor[i]->SetVisibility(textActorShouldBeVisible);
        }
        else
        {
            m_patientOrientationTextActor[i]->SetVisibility(false);
        }
    }
}

void Q2DViewerAnnotationHandler::refreshAnnotations()
{
    if (!m_2DViewer->hasInput())
    {
        return;
    }

    if (m_enabledAnnotations.testFlag(PatientInformationAnnotation))
    {
        setCornerAnnotation(UpperRightCornerIndex, m_upperRightText);
        setCornerAnnotation(LowerRightCornerIndex, m_lowerRightText.trimmed());
    }
    else
    {
        setCornerAnnotation(UpperRightCornerIndex, " ");
        setCornerAnnotation(LowerRightCornerIndex, " ");
    }

    if (m_enabledAnnotations.testFlag(PatientOrientationAnnotation))
    {
        for (int j = 0; j < 4; j++)
        {
            if (!m_patientOrientationText[j].isEmpty())
            {
                m_patientOrientationTextActor[j]->VisibilityOn();
            }
            else
            {
                // If label is empty, disable visibility as we don't have nothing to show
                m_patientOrientationTextActor[j]->VisibilityOff();
            }
        }
    }
    else
    {
        for (int j = 0; j < 4; j++)
        {
            m_patientOrientationTextActor[j]->VisibilityOff();
        }
    }

    updateAnnotationsInformation(VoiLutInformationAnnotation | SliceAnnotation);
}

void Q2DViewerAnnotationHandler::updateSliceAnnotation()
{
    Q_ASSERT(m_cornerAnnotations);

    if (m_enabledAnnotations.testFlag(SliceAnnotation))
    {
        QString lowerLeftText;
        
        lowerLeftText = getSliceLocationAnnotation();
        
        // Setup the slice/slab annotation
        lowerLeftText += QObject::tr("Slice: %1").arg(m_2DViewer->getCurrentSlice() + 1);
        if (m_2DViewer->isThickSlabActive())
        {
            // TODO We need a getLastSlabSlice() method on Q2Dviewer to avoid doing this computing
            lowerLeftText += QObject::tr("-%1").arg(m_2DViewer->getCurrentSlice() + m_2DViewer->getSlabThickness());
        }
        lowerLeftText += QObject::tr("/%1").arg(m_2DViewer->getNumberOfSlices());
        
        // If we have phases
        if (m_2DViewer->hasPhases())
        {
            lowerLeftText += QObject::tr(" Phase: %1/%2").arg(m_2DViewer->getCurrentPhase() + 1).arg(m_2DViewer->getNumberOfPhases());
        }
        
        // Add slice thickness only if it is > 0.0mm
        if (m_2DViewer->getCurrentSliceThickness() > 0.0)
        {
            lowerLeftText += QObject::tr(" Thickness: %1 mm").arg(m_2DViewer->getCurrentSliceThickness(), 0, 'f', 2);
        }

        setCornerAnnotation(LowerLeftCornerIndex, lowerLeftText);
    }
    else
    {
        setCornerAnnotation(LowerLeftCornerIndex, " ");
    }
}

void Q2DViewerAnnotationHandler::updateLateralityAnnotationInformation()
{
    QChar laterality = m_2DViewer->getCurrentDisplayedImageLaterality();
    if (!laterality.isNull() && !laterality.isSpace())
    {
        QString lateralityAnnotation = "Lat: " + QString(laterality);
            
        if (m_lowerRightText.trimmed().isEmpty())
        {
            setCornerAnnotation(LowerRightCornerIndex, lateralityAnnotation);
        }
        else
        {
            setCornerAnnotation(LowerRightCornerIndex, (lateralityAnnotation + "\n" + m_lowerRightText.trimmed()));
        }
    }
    else
    {
        setCornerAnnotation(LowerRightCornerIndex, m_lowerRightText.trimmed());
    }
}

void Q2DViewerAnnotationHandler::updatePatientInformationAnnotation()
{
    if (m_enabledAnnotations.testFlag(PatientInformationAnnotation))
    {
        // If we are viewing the original acquisition and acquisition time is present, show it as well
        if (m_2DViewer->getView() == OrthogonalPlane::XYPlane)
        {
            Image *currentImage = m_2DViewer->getCurrentDisplayedImage();
            if (currentImage)
            {
                QString imageTime = "\n" + currentImage->getFormattedImageTime();
                if (imageTime.isEmpty())
                {
                    imageTime = "--:--";
                }
                setCornerAnnotation(UpperRightCornerIndex, (m_upperRightText + imageTime));
            }
            else
            {
                setCornerAnnotation(UpperRightCornerIndex, m_upperRightText);
            }
        }
        else
        {
            setCornerAnnotation(UpperRightCornerIndex, m_upperRightText);
        }
    }
}

void Q2DViewerAnnotationHandler::updateVoiLutInformationAnnotation()
{
    if (m_enabledAnnotations.testFlag(VoiLutInformationAnnotation))
    {
        int dimensions[3];
        m_2DViewer->getMainInput()->getDimensions(dimensions);
        int xIndex = m_2DViewer->getView().getXIndex();
        int yIndex = m_2DViewer->getView().getYIndex();
        m_upperLeftText = QObject::tr("%1 x %2").arg(dimensions[xIndex]).arg(dimensions[yIndex]);
        m_upperLeftText += "\n";
        m_upperLeftText += getVoiLutString();
    }
    else
    {
        m_upperLeftText = " ";
    }
    
    setCornerAnnotation(UpperLeftCornerIndex, m_upperLeftText);
}

QString Q2DViewerAnnotationHandler::getSliceLocationAnnotation()
{
    QString sliceLocation;
    
    // Slice location will be present only when we are on the original acquisition plane
    if (m_2DViewer->getView() == OrthogonalPlane::XYPlane)
    {
        Image *image = m_2DViewer->getCurrentDisplayedImage();
        if (image)
        {
            QString location = image->getSliceLocation();
            if (!location.isEmpty())
            {
                sliceLocation = QObject::tr("Loc: %1").arg(location.toDouble(), 0, 'f', 2);
                if (m_2DViewer->isThickSlabActive())
                {
                    // TODO We should have high level methods to get consecutive images according to current thickness, phase, etc.
                    Image *secondImage = m_2DViewer->getMainInput()->getImage(
                        // TODO We need a getLastSlabSlice() method on Q2Dviewer to avoid doing this computing
                        m_2DViewer->getCurrentSlice() + m_2DViewer->getSlabThickness() - 1,
                        m_2DViewer->getCurrentPhase());
                    if (secondImage)
                    {
                        sliceLocation += QObject::tr("-%1").arg(secondImage->getSliceLocation().toDouble(), 0, 'f', 2);
                    }
                }
                sliceLocation += "\n";
            }
        }
    }

    return sliceLocation;
}

QString Q2DViewerAnnotationHandler::getSeriesDescriptiveLabel(Series *series) const
{
    if (!series)
    {
        return QString();
    }
    
    // If protocol and description are equal, protocol will be set, otherwise they will be merged
    QString protocolName = series->getProtocolName();
    QString description = series->getDescription();
    
    QString label = protocolName;
    
    if (description != protocolName)
    {
        label += "\n" + description;
    }

    return label;
}

void Q2DViewerAnnotationHandler::createAnnotations()
{
    m_cornerAnnotations = vtkCornerAnnotation::New();
    m_cornerAnnotations->GetTextProperty()->SetFontFamilyToArial();
    m_cornerAnnotations->GetTextProperty()->ShadowOn();

    createOrientationAnnotations();
}

void Q2DViewerAnnotationHandler::createOrientationAnnotations()
{
    for (int i = 0; i < 4; i++)
    {
        m_patientOrientationTextActor[i] = vtkTextActor::New();
        m_patientOrientationTextActor[i]->SetTextScaleModeToNone();
        m_patientOrientationTextActor[i]->GetTextProperty()->SetFontSize(18);
        m_patientOrientationTextActor[i]->GetTextProperty()->BoldOn();
        m_patientOrientationTextActor[i]->GetTextProperty()->SetFontFamilyToArial();
        m_patientOrientationTextActor[i]->GetTextProperty()->ShadowOn();

        m_patientOrientationTextActor[i]->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
        m_patientOrientationTextActor[i]->GetPosition2Coordinate()->SetCoordinateSystemToNormalizedViewport();
    }
    // Place each actor on its corresponding place. 0-3, counter-clockwise direction, starting at 0 = left of the viewer
    m_patientOrientationTextActor[LeftOrientationLabelIndex]->GetTextProperty()->SetJustificationToLeft();
    m_patientOrientationTextActor[LeftOrientationLabelIndex]->SetPosition(0.01, 0.5);

    m_patientOrientationTextActor[BottomOrientationLabelIndex]->GetTextProperty()->SetJustificationToCentered();
    m_patientOrientationTextActor[BottomOrientationLabelIndex]->SetPosition(0.5, 0.01);

    m_patientOrientationTextActor[RightOrientationLabelIndex]->GetTextProperty()->SetJustificationToRight();
    m_patientOrientationTextActor[RightOrientationLabelIndex]->SetPosition(0.99, 0.5);

    m_patientOrientationTextActor[TopOrientationLabelIndex]->GetTextProperty()->SetJustificationToCentered();
    m_patientOrientationTextActor[TopOrientationLabelIndex]->GetTextProperty()->SetVerticalJustificationToTop();
    m_patientOrientationTextActor[TopOrientationLabelIndex]->SetPosition(0.5, 0.99);
}

void Q2DViewerAnnotationHandler::addActors()
{
    Q_ASSERT(m_cornerAnnotations);
    Q_ASSERT(m_patientOrientationTextActor[0]);
    Q_ASSERT(m_patientOrientationTextActor[1]);
    Q_ASSERT(m_patientOrientationTextActor[2]);
    Q_ASSERT(m_patientOrientationTextActor[3]);

    vtkRenderer *renderer = m_2DViewer->getRenderer();
    Q_ASSERT(renderer);
    
    renderer->AddViewProp(m_cornerAnnotations);
    renderer->AddViewProp(m_patientOrientationTextActor[0]);
    renderer->AddViewProp(m_patientOrientationTextActor[1]);
    renderer->AddViewProp(m_patientOrientationTextActor[2]);
    renderer->AddViewProp(m_patientOrientationTextActor[3]);
}

QString Q2DViewerAnnotationHandler::getVoiLutString() const
{
    VoiLut voiLut = m_2DViewer->getCurrentVoiLut();
    QString lutPart;

    if (voiLut.isLut())
    {
        lutPart = voiLut.getOriginalLutExplanation() + " ";
    }

    WindowLevel windowLevel = voiLut.getWindowLevel();
    QString windowLevelPart = QObject::tr("WW: %1 WL: %2").arg(MathTools::roundToNearestInteger(windowLevel.getWidth()))
                                                          .arg(MathTools::roundToNearestInteger(windowLevel.getCenter()));

    QString thresholdPart;

    if (VolumeHelper::isPrimaryPET(m_2DViewer->getMainInput()) || VolumeHelper::isPrimaryNM(m_2DViewer->getMainInput()))
    {
        double range[2];
        m_2DViewer->getMainInput()->getScalarRange(range);

        double percent = 0.0;
        // Avoid division by zero
        if (range[1] != 0.0)
        {
            percent = (windowLevel.getWidth() / range[1]) * 100;
        }

        thresholdPart = "\n" + QObject::tr("Threshold: %1%").arg(percent, 0, 'f', 2);
    }

    return lutPart + windowLevelPart + thresholdPart;
}

void Q2DViewerAnnotationHandler::setCornerAnnotation(CornerAnnotationIndexType corner, const QString &text)
{
    MammographyImageHelper mammographyImageHelper;
    Image *image = m_2DViewer->getCurrentDisplayedImage();

    // If displaying a mammography and the posterior side is at the right, then swap annotation sides so that patient information doesn't cover the image
    if (mammographyImageHelper.isStandardMammographyImage(image) &&
            m_2DViewer->getCurrentDisplayedImagePatientOrientation().getRowDirectionLabel() == PatientOrientation::PosteriorLabel)
    {
        switch (corner)
        {
            case LowerLeftCornerIndex: corner = LowerRightCornerIndex; break;
            case LowerRightCornerIndex: corner = LowerLeftCornerIndex; break;
            case UpperLeftCornerIndex: corner = UpperRightCornerIndex; break;
            case UpperRightCornerIndex: corner = UpperLeftCornerIndex; break;
        }
    }

    m_cornerAnnotations->SetText(corner, text.toUtf8().constData());
}

} // End namespace udg

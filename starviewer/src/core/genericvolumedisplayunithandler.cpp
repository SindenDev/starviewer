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

#include "genericvolumedisplayunithandler.h"

#include "mathtools.h"
#include "logging.h"
#include "volumedisplayunit.h"
#include "imagepipeline.h"
#include "transferfunctionmodel.h"

#include <limits>

#include <vtkImageActor.h>

namespace udg {

GenericVolumeDisplayUnitHandler::GenericVolumeDisplayUnitHandler()
{
    m_transferFunctionModel = new TransferFunctionModel();
}

GenericVolumeDisplayUnitHandler::~GenericVolumeDisplayUnitHandler()
{
    removeDisplayUnits();
    delete m_transferFunctionModel;
}

void GenericVolumeDisplayUnitHandler::setInput(Volume *input)
{
    removeDisplayUnits();
    addDisplayUnit(input);
    setupDisplayUnits();
}

void GenericVolumeDisplayUnitHandler::setInputs(QList<Volume*> inputs)
{
    removeDisplayUnits();

    int supportedNumberOfInputs = qBound(0, inputs.size(), getMaximumNumberOfInputs());
    
    if (supportedNumberOfInputs < inputs.size())
    {
        DEBUG_LOG(QString("The number of the given inputs (%1) exceeds the maximum of the handler (%2). There will be only %2 handled.")
            .arg(inputs.size()).arg(getMaximumNumberOfInputs()));
    }
    
    for (int i = 0; i < supportedNumberOfInputs; ++i)
    {
        addDisplayUnit(inputs.at(i));
    }
    setupDisplayUnits();
}

bool GenericVolumeDisplayUnitHandler::hasInput() const
{
    return getNumberOfInputs() > 0;
}

int GenericVolumeDisplayUnitHandler::getNumberOfInputs() const
{
    return m_displayUnits.count();
}

VolumeDisplayUnit* GenericVolumeDisplayUnitHandler::getMainVolumeDisplayUnit() const
{
    return getVolumeDisplayUnit(0);
}

VolumeDisplayUnit* GenericVolumeDisplayUnitHandler::getVolumeDisplayUnit(int i) const
{
    if (MathTools::isInsideRange(i, 0, m_displayUnits.count() - 1))
    {
        return m_displayUnits.at(i);
    }
    else
    {
        return 0;
    }
}

QList<VolumeDisplayUnit*> GenericVolumeDisplayUnitHandler::getVolumeDisplayUnitList() const
{
    return m_displayUnits;
}

int GenericVolumeDisplayUnitHandler::getMaximumNumberOfInputs() const
{
    return std::numeric_limits<int>::max();
}

TransferFunctionModel* GenericVolumeDisplayUnitHandler::getTransferFunctionModel() const
{
    return m_transferFunctionModel;
}

void GenericVolumeDisplayUnitHandler::removeDisplayUnits()
{
    foreach (VolumeDisplayUnit *unit, m_displayUnits)
    {
        delete unit;
    }
    m_displayUnits.clear();
}

void GenericVolumeDisplayUnitHandler::addDisplayUnit(Volume *input)
{
    if (!input)
    {
        return;
    }

    VolumeDisplayUnit *displayUnit = new VolumeDisplayUnit();
    // Add the vdu to the list before setting the volume to avoid a memory leak if setVolume throws a bad_alloc
    m_displayUnits << displayUnit;
    displayUnit->setVolume(input);
}

void GenericVolumeDisplayUnitHandler::setupDisplayUnits()
{
    updateMainDisplayUnitIndex();
    setupDefaultOpacities();
    initializeTransferFunctions();
}

void GenericVolumeDisplayUnitHandler::setupDefaultOpacities()
{
    // The default opacities will be 1 for the main volume and 0.5 for the others
    for (int i = 0; i < getNumberOfInputs(); i++)
    {
        vtkImageActor *actor = m_displayUnits.at(i)->getImageActor();
        actor->SetOpacity(1.0);
    }
}

void GenericVolumeDisplayUnitHandler::initializeTransferFunctions()
{
    // By default, all volumes have no transfer function
    foreach (VolumeDisplayUnit *unit, m_displayUnits)
    {
        unit->clearTransferFunction();
    }

    setupDefaultTransferFunctions();
}

} // End namespace udg

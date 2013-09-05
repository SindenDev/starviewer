#ifndef UDGQ2DVIEWER_H
#define UDGQ2DVIEWER_H

#include "qviewer.h"

#include <QPointer>

// Fordward declarations
// Vtk
class vtkPropPicker;
class vtkTextActor;
class vtkCornerAnnotation;
class vtkCoordinate;
class vtkImageActor;
class vtkImageData;

namespace udg {

// Fordward declarations
class Image;
class ImageOverlay;
class Drawer;
class DrawerBitmap;
class ImagePlane;
class ImageOrientationOperationsMapper;
class VolumeReaderManager;
class QViewerCommand;
class PatientOrientation;
class BlendFilter;
class VolumeDisplayUnit;
class VolumePixelData;

/**
    Classe base per als visualitzadors 2D.

    El mode d'operació habitual serà el de visualitar un sol volum.
    Normalment per poder visualitzar un volum farem el següent
    \code
    Q2DViewer* visor = new Q2DViewer();
    visor->setInput(volum);

    En el cas que desitjem solapar dos volums haurem d'indicar el volum solapat amb el mètode setOverlayInput().
    Quan solapem volums tenim 1 manera de solapar aquests volums, aplicant un blending,
    en aquest cas hauríem de fer servir el mètode setOverlapMethod() indicant una de les opcions (de moment únicament Blend)
    \TODO acabar la doc sobre solapament

    Per defecte el visualitzador mostra la primera imatge en Axial. Per les altres vistes (Sagital i Coronal) mostraria la imatge central

    Podem escollir quines annotacions textuals i de referència apareixeran en la vista 2D a través dels flags "AnnotationFlags" definits com enums.
    Aquests flags es poden passar en el constructor o els podem modificar a través dels mètodes \c addAnnotation() o \c removeAnnotation()
    que faran visible o invisible l'anotació indicada. Per defecte el flag és \c AllAnnotation i per tant es veuen totes les anotacions per defecte.
  */
class Q2DViewer : public QViewer {
Q_OBJECT
public:
    /// Tipus de solapament dels models
    enum OverlapMethod { None, Blend };

    /// Alineament de la imatge (dreta, esquerre, centrat)
    enum AlignPosition { AlignCenter, AlignRight, AlignLeft };

    /// Aquests flags els farem servir per decidir quines anotacions seran visibles i quines no
    enum AnnotationFlag { NoAnnotation = 0x0, WindowInformationAnnotation = 0x1, PatientOrientationAnnotation = 0x2, SliceAnnotation = 0x8,
                          PatientInformationAnnotation = 0x10, AcquisitionInformationAnnotation = 0x20, AllAnnotation = 0x7F };
    Q_DECLARE_FLAGS(AnnotationFlags, AnnotationFlag)

    Q2DViewer(QWidget *parent = 0);
    ~Q2DViewer();

    /// Ens retorna la vista que tenim en aquells moments del volum
    const OrthogonalPlane& getView() const;

    /// Assigna/Retorna el volum solapat
    void setOverlayInput(Volume *volume);
    Volume* getOverlayInput();

    /// Indiquem que cal actualitzar l'Overlay actual
    void updateOverlay();

    /// Assignem l'opacitat del volum solapat.
    /// Els valors podran anar de 0.0 a 1.0, on 0.0 és transparent i 1.0 és completament opac.
    void setOverlayOpacity(double opacity);

    /// Obté el window level actual de la imatge
    void getCurrentWindowLevel(double wl[2]);

    /// Retorna la llesca/fase actual
    int getCurrentSlice() const;
    int getCurrentPhase() const;

    /// Ens retorna el drawer per poder pintar-hi primitives
    /// @return Objecte drawer del viewer
    Drawer* getDrawer() const;

    /// Calcula la coordenada de la imatge que es troba per sota del cursor en coordenades de món
    /// En el cas el cursor estigui fora de la imatge, la coordenada no té cap validesa
    /// @param xyz[] La coordenada de la imatge, en sistema de coordenades de món
    /// @return Cert si el cursor es troba dins de la imatge, fals altrament
    bool getCurrentCursorImageCoordinate(double xyz[3]);

    /// Returns current displayed image.
    /// If some orthogonal reconstruction different from original acquisition is applied, returns null
    Image* getCurrentDisplayedImage() const;

    /// Ens dóna el pla d'imatge actual que estem visualitzant
    /// @param vtkReconstructionHack HACK variable booleana que ens fa un petit hack
    /// per casos en que el pla "real" no és el que volem i necessitem una petita modificació
    /// ATENCIÓ: Només es donarà aquest paràmetre (amb valor true) en casos que realment se sàpiga el que s'està fent!
    /// @return El pla imatge actual
    ImagePlane* getCurrentImagePlane(bool vtkReconstructionHack = false);

    /// donat un punt 3D en espai de referència DICOM, ens dóna la projecció d'aquest punt sobre
    /// el pla actual, transformat a coordenades de món VTK
    /// @param pointToProject[]
    /// @param projectedPoint[]
    /// @param vtkReconstructionHack HACK variable booleana que ens fa un petit hack
    /// per casos en que el pla "real" no és el que volem i necessitem una petita modificació
    void projectDICOMPointToCurrentDisplayedImage(const double pointToProject[3], double projectedPoint[3], bool vtkReconstructionHack = false);

    /// Retorna el thickness. En cas que no disposem del thickness, el valor retornat serà 0.0
    double getCurrentSliceThickness() const;

    /// Ens dóna la llesca mínima/màxima de llesques, tenint en compte totes les imatges,
    /// tant com si hi ha fases com si no
    /// @return valor de la llesca mínima/màxima
    int getMinimumSlice() const;
    int getMaximumSlice() const;

    /// Returns the total number of slices on the spatial dimension that has the main input on the current view
    int getNumberOfSlices() const;

    /// Ens indica si s'està aplicant o no thick slab
    bool isThickSlabActive() const;

    /// Obtenim el mode de projecció del thickslab.
    /// Si el thickslab no està actiu, el valor és indefinit
    int getSlabProjectionMode() const;

    /// Obtenim el gruix de l'slab
    /// Si el thickslab no està actiu, el valor és indefinit
    int getSlabThickness() const;

    /// Donada una coordenada de món, l'ajustem perquè caigui dins dels límits de l'imatge actual
    /// Això ens serveix per tools que agafen qualsevol punt de món, però necessiten que aquesta estigui
    /// dins dels límits de la imatge, com pot ser una ROI. Aquest mètode acaba d'ajustar la coordenada perquè
    /// estigui dins dels límits de la pròpia imatge
    /// @param xyz[] Coordenada que volem ajustar. Serà un paràmetre d'entrada/sortida i el seu contingut
    /// es modificarà perquè caigui dins dels límits de la imatge
    void putCoordinateInCurrentImageBounds(double xyz[3]);

    /// Gets the pixel data corresponding to the current rendered image
    VolumePixelData* getCurrentPixelData();

    /// Retorna la orientació de pacient corresponent a la imatge que s'està visualitzant en aquell moment,
    /// és a dir, tenint en compte rotacions, flips, reconstruccions, etc.
    PatientOrientation getCurrentDisplayedImagePatientOrientation() const;

    /// Ens diu quin és el pla de projecció de la imatge que es veu en aquell moment
    /// Valors: AXIAL, SAGITAL, CORONAL, OBLIQUE o N/A
    QString getCurrentAnatomicalPlaneLabel() const;

    /// Retorna l'espai que hi ha entre les llesques segons la vista actual i si hi ha el thickness activat
    double getCurrentSpacingBetweenSlices();

    /// Ens retorna l'acttor vtk de la imatge
    vtkImageActor* getVtkImageActor() const;

    /// Returns true if this Q2DViewer can show a display shutter in its current state, i.e. if there is a display shutter for the current image and there isn't
    /// any restriction to show display shutters.
    bool canShowDisplayShutter() const;
    
    /// Casts the given QViewer to a Q2DViewer object
    /// If casting is successful, casted pointer to Q2DViewer will be returned, null otherwise
    static Q2DViewer* castFromQViewer(QViewer *viewer);

    /// Sets the opacity of the image actor of the volume at the given index.
    void setVolumeOpacity(int index, double opacity);

public slots:
    virtual void setInput(Volume *volume);

    /// Especifica el volum d'entrada de forma asíncrona.
    /// Es pot indicar un command que s'executarà un cop el volum s'ha carregat i està a punt de ser visualitzat.
    /// Útil per poder especificar canvis al viewer (canvi de llesca, w/l, etc.) sense preocupar-se de quan s'ha carregat el volume.
    void setInputAsynchronously(Volume *volume, QViewerCommand *inputFinishedCommand = 0);

    void resetView(const OrthogonalPlane &view);

    /// Restaura el visualitzador a l'estat inicial
    void restore();

    /// Esborra totes les primitives del visor
    void clearViewer();

    /// Canvia el WW del visualitzador, per tal de canviar els blancs per negres, i el negres per blancs
    void invertWindowLevel();

    /// Canvia la llesca que veiem de la vista actual
    void setSlice(int value);

    /// Canvia la fase en que es veuen les llesques si n'hi ha
    void setPhase(int value);

    /// Indica el tipu de solapament dels volums, per defecte blending
    void setOverlapMethod(OverlapMethod method);

    /// Afegir o treure la visibilitat d'una anotació textual/gràfica
    void enableAnnotation(AnnotationFlags annotation, bool enable = true);
    void removeAnnotation(AnnotationFlags annotation);

    void setWindowLevel(double window, double level);
    void setTransferFunction(TransferFunction *transferFunction);

    /// L'únic que fa és emetre el senyal seedPositionChanged, per poder-ho cridar desde la seedTool
    /// TODO Aquest mètode hauria de quedar obsolet
    void setSeedPosition(double pos[3]);

    /// Aplica una rotació de 90 graus en el sentit de les agulles del rellotge
    /// tantes "times" com li indiquem, per defecte sempre serà 1 "time"
    void rotateClockWise(int times = 1);

    /// Aplica una rotació de 90 graus en el sentit contrari a les agulles del rellotge
    /// tantes "times" com li indiquem, per defecte sempre serà 1 "time"
    void rotateCounterClockWise(int times = 1);

    /// Aplica un flip horitzontal/vertical sobre la imatge. El flip vertical es farà com una rotació de 180º seguida d'un flip horitzontal
    void horizontalFlip();
    void verticalFlip();

    // TODO aquests mètodes també haurien d'estar en versió QString!

    /// Li indiquem quin mode de projecció volem aplicar sobre l'slab
    /// @param projectionMode Valor que identifica quina projecció apliquem
    void setSlabProjectionMode(int projectionMode);

    /// Indiquem el gruix de l'slab
    /// @param thickness Nombre de llesques que formen l'slab
    void setSlabThickness(int thickness);

    /// Disables thick slab. Acts as a shortcut for setSlabThickness(1)
    void disableThickSlab();

    /// Alineament de la imatge dins del visualitzador
    void alignLeft();
    void alignRight();

    /// Posa la posició d'alineament de la imatge (dreta, esquerre, centrat)
    void setAlignPosition(AlignPosition alignPosition);

    /// Aplica les transformacions 2D necessàries sobre la imatge actual perquè aquesta tingui la orientació indicada
    /// La orientació indicada ha de ser possible de conseguir mitjançant operacions de rotació i flip. En cas que no
    /// existeixin combinacions possibles, no es canviarà la orientació de la imatge
    void setImageOrientation(const PatientOrientation &desiredPatientOrientation);

    /// Fa que els ImageOverlays siguin visibles o no
    void showImageOverlays(bool enable);

    /// Fa que els shutters siguin visibles o no
    void showDisplayShutters(bool enable);

signals:
    /// Envia la nova llesca en la que ens trobem
    void sliceChanged(int);

    /// Envia la nova fase en la que ens trobem
    void phaseChanged(int);

    /// Envia la nova vista en la que ens trobem
    void viewChanged(int);

    /// Indica el nou window level
    void windowLevelChanged(double window, double level);

    /// Emitted when a new patient orientation has been set
    void imageOrientationChanged(const PatientOrientation &orientation);
    
    /// Senyal que s'envia quan la llavor s'ha canviat
    /// TODO Mirar de treure-ho i posar-ho en la tool SeedTool
    void seedPositionChanged(double x, double y, double z);

    /// S'emet quan canvia l'slab thickness
    /// @param thickness Nou valor de thickness
    void slabThicknessChanged(int thickness);

    /// Senyal que s'envia quan ha canviat l'overlay
    void overlayChanged();
    void overlayModified();

protected:
    /// Processem l'event de resize de la finestra Qt
    virtual void resizeEvent(QResizeEvent *resize);

    void getCurrentRenderedItemBounds(double bounds[6]);

    void setDefaultOrientation(AnatomicalPlane::AnatomicalPlaneType anatomicalPlane);

    /// Returns the current view plane.
    virtual const OrthogonalPlane& getCurrentViewPlane() const;

    /// Sets the current view plane.
    virtual void setCurrentViewPlane(const OrthogonalPlane &viewPlane);

private:
    /// Updates image orientation according to the preferred presentation depending on its attributes, like modality.
    /// At this moment it is only applying to mammography (MG) images
    void updatePreferredImageOrientation();
    
    /// Refresca la visibilitat de les annotacions en funció dels flags que tenim
    void refreshAnnotations();

    /// Actualitzem les dades de les annotacions, per defecte totes, sinó, només les especificades
    void updateAnnotationsInformation(AnnotationFlags annotation = Q2DViewer::AllAnnotation);

    /// Desglossem les actualitzacions de les diferents informacions que es mostren per pantalla
    void updatePatientAnnotationInformation();
    void updateSliceAnnotationInformation();
    void updateLateralityAnnotationInformation();
    void updatePatientInformationAnnotation();

    /// Returns the laterality corresponding to the current displayed image.
    /// If image is not reconstructed, image laterality is returned, or series laterality if not present
    /// If image is reconstructed, series laterality is returned
    /// If no laterality is found, en empty character will be returned
    QChar getCurrentDisplayedImageLaterality() const;
    
    /// Refresca els valors de les annotacions de llesca. Si els valors referents
    /// a les fases són < 2 no es printarà informació de fases
    /// Si hi ha thick slab, mostrarà el rang d'aquest
    void updateSliceAnnotation(int currentSlice, int maxSlice, int currentPhase = 0, int maxPhase = 0);

    /// Crea i inicialitza totes les anotacions que apareixeran per pantalla
    void createAnnotations();

    /// Crea les anotacions de l'orientació del pacient
    void createOrientationAnnotations();

    /// Afegeix tots els actors a l'escena
    void addActors();

    /// Actualitza les etiquetes d'orientació del pacient segons la vista i orientació actuals de la càmera
    void updatePatientOrientationAnnotation();

    /// Updates the display extents of the image actors.
    void updateDisplayExtents();

    /// Print some information related to the volume
    void printVolumeInformation();

    /// Actualitza el pipeline del filtre de shutter segons si està habilitat o no
    void updateShutterPipeline();

    /// Updates the mask used as display shutter if display shutters should and can be shown.
    void updateDisplayShutterMask();

    /// Re-inicia els paràmetres de la càmera segons la vista actual.
    void resetCamera();

    /// Aplica el factor de rotació adient segons els girs que li indiquem. No actualitza la càmera ni l'escena, simplement
    /// es fa servir per posar els valors correctes a les variables internes que controlen com està girada la imatge.
    void rotate(int times);

    /// Sets if image should be flipped (horizontally) or not. It does not update the camera nor renders the scene.
    void setFlip(bool flip);
    
    /// Updates the camera, renders and emits the current image orientataion
    void applyImageOrientationChanges();
    
    /// Carrega un volum asíncronament
    void loadVolumeAsynchronously(Volume *volume);

    /// Retorna un volum "dummy"
    Volume* getDummyVolumeFromVolume(Volume *volume);

    /// Especifica quin command s'ha d'executar després d'especificar un volum com a input
    void setInputFinishedCommand(QViewerCommand *command);

    /// Elimina el command que s'hauria d'executar després de que s'especifiqui un input
    void deleteInputFinishedCommand();

    /// Si està definit, executa el command definit per després d'especificar un input al viewer
    void executeInputFinishedCommand();

    /// Updates the current image default presets values. It only applies to original acquisition plane.
    void updateCurrentImageDefaultPresets();

    /// Calls setNewVolumes and excutes the command while catching any exception that may be thrown.
    void setNewVolumesAndExecuteCommand(const QList<Volume*> &volumes);

    /// Elimina els bitmaps que s'hagin creat per aquest viewer
    void removeViewerBitmaps();
    
    /// Carrega en memòria els ImageOverlays del volum passat per paràmetre (sempre que no sigui un dummy) i els afegeix al Drawer
    void loadOverlays(Volume *volume);

    /// Enum to define the different dimensions an image slice could be associated to
    typedef enum SliceDimension { SpatialDimension, TemporalDimension };
    /// Updates the image slice to be displayed on the specified dimension
    void updateSliceToDisplay(int value, SliceDimension dimension);

    /// Updates the slice to display in the secondary volumes to the closest one in the main volume.
    void updateSecondaryVolumesSlices();

    /// Creates or destroys volume display units as needed according to the new number of volumes. Also, adds or removes image actors from the viewer.
    void setupVolumeDisplayUnits(int count);

private slots:
    /// Actualitza les transformacions de càmera (de moment rotació i flip)
    void updateCamera();

    /// Reimplementem per tal de que faci un setInputAsynchronously
    /// TODO: De moment es fa així de manera xapussa fins que no es traspassin els mètode de càrrega
    /// asíncrona a QViewer.
    virtual void setInputAndRender(Volume *volume);

    /// Replaces the volumes displayed by this viewer by the new ones and resets the viewer.
    /// If the second parameter is false, the volumes won't be rendered.
    void setNewVolumes(const QList<Volume*> &volumes, bool setViewerStatusToVisualizingVolume = true);

    void volumeReaderJobFinished();

protected:
    /// Aquest és el segon volum afegit a solapar
    Volume *m_overlayVolume;

    /// Aquest és el blender per veure imatges fusionades
    BlendFilter* m_blender;

    /// Opacitat del volum solapat
    double m_overlayOpacity;

    /// El picker per agafar punts de la imatge
    vtkPropPicker *m_imagePointPicker;

    /// Annotacions de texte referents a informació de la sèrie
    /// (nom de pacient, protocol,descripció de sèrie, data de l'estudi, etc)
    /// i altre informació rellevant (nº imatge, ww/wl, etc)
    vtkCornerAnnotation *m_cornerAnnotations;

private:
    /// Nom del grups dins del drawer per als Overlays
    static const QString OverlaysDrawerGroup;

    /// Constant per a definir el nom d'objecte dels volums "dummy"
    static const QString DummyVolumeObjectName;
    
    /// Flag que ens indica quines anotacions es veuran per la finestra
    AnnotationFlags m_enabledAnnotations;

    /// Tipus de solapament dels volums en cas que en tinguem més d'un
    OverlapMethod m_overlapMethod;

    /// Els strings amb els textes de cada part de la imatge
    QString m_lowerLeftText, m_lowerRightText, m_upperLeftText, m_upperRightText;

    /// Aquest string indica les anotacions que ens donen les referències del pacient (Right,Left,Posterior,Anterior,Inferior,Superior)
    QString m_patientOrientationText[4];

    /// Textes adicionals d'anotoació
    vtkTextActor *m_patientOrientationTextActor[4];

    /// Factor de rotació. En sentit de les agulles del rellotge 0: 0º, 1: 90º, 2: 180º, 3: 270º.
    int m_rotateFactor;

    /// Indica si cal aplicar un flip horitzontal o no sobre la càmera
    bool m_applyFlip;

    /// Aquesta variable controla si la imatge està flipada respecte la seva orientació original. Útil per controlar annotacions.
    bool m_isImageFlipped;

    /// Especialista en dibuixar primitives
    Drawer *m_drawer;

    /// Indica quin tipus de projecció apliquem sobre l'slab
    int m_slabProjectionMode;

    /// Conté el mapeig d'operacions a fer quan voelm passar d'una orientació a un altre
    ImageOrientationOperationsMapper *m_imageOrientationOperationsMapper;

    /// Posició a on s'ha d'alinear la imatge (dreta, esquerre o centrat)
    AlignPosition m_alignPosition;

    /// Manager of the reading of volumes
    VolumeReaderManager *m_volumeReaderManager;

    QViewerCommand *m_inputFinishedCommand;

    /// Llistat d'overlays
    QList<DrawerBitmap*> m_viewerBitmaps;

    /// Controla si els overlays estan habilitats o no
    bool m_overlaysAreEnabled;
    
    /// If true, display shutters are visible when they are available and it's possible to show them.
    bool m_showDisplayShutters;

    /// Volume display units containing the volumes displayed in this viewer and their related objects.
    QList<VolumeDisplayUnit*> m_volumeDisplayUnits;

    /// Holds the current thickslab pixel data
    VolumePixelData *m_currentThickSlabPixelData;

};
Q_DECLARE_OPERATORS_FOR_FLAGS(Q2DViewer::AnnotationFlags)
};  //  End namespace udg

#endif

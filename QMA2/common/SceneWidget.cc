/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2009-2011  Nagoya Institute of Technology          */
/*                           Department of Computer Science          */
/*                2010-2012  hkrn                                    */
/*                                                                   */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/* - Redistributions of source code must retain the above copyright  */
/*   notice, this list of conditions and the following disclaimer.   */
/* - Redistributions in binary form must reproduce the above         */
/*   copyright notice, this list of conditions and the following     */
/*   disclaimer in the documentation and/or other materials provided */
/*   with the distribution.                                          */
/* - Neither the name of the MMDAI project team nor the names of     */
/*   its contributors may be used to endorse or promote products     */
/*   derived from this software without specific prior written       */
/*   permission.                                                     */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            */
/* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          */
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/* ----------------------------------------------------------------- */

/* for GLEW limitation, include vpvl.h first to define VPVL_LINK_GLEW except Darwin */
#include <vpvl/vpvl.h>

#include "SceneWidget.h"

#include "Application.h"
#include "DebugDrawer.h"
#include "Delegate.h"
#include "Grid.h"
#include "Handles.h"
#include "InfoPanel.h"
#include "SceneLoader.h"
#include "World.h"
#include "util.h"

#include <QtGui/QtGui>
#include <btBulletDynamicsCommon.h>

#ifdef Q_OS_DARWIN
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#ifdef VPVL_ENABLE_GLSL
#include <vpvl/gl2/Renderer.h>
using namespace vpvl::gl2;
#else
#include <vpvl/gl/Renderer.h>
using namespace vpvl::gl;
#endif /* VPVL_ENABLE_GLSL */
using namespace internal;

namespace {

typedef QSharedPointer<QProgressDialog> ProgressDialogPtr;

ProgressDialogPtr UIGetProgressDialog(const QString &label, int max)
{
    QProgressDialog *progress = new QProgressDialog(label, QApplication::tr("Cancel"), 0, max);
    progress->setMinimumDuration(0);
    progress->setRange(0, 1);
    progress->setValue(0);
    progress->setWindowModality(Qt::WindowModal);
    return ProgressDialogPtr(progress);
}

#ifdef GL_MULTISAMPLE
static inline void EnableMultisample()
{
    glEnable(GL_MULTISAMPLE);
}
#else
#define EnableMultisample() (void) 0
#endif

}

SceneWidget::SceneWidget(QSettings *settings, QWidget *parent) :
    QGLWidget(QGLFormat(QGL::SampleBuffers), parent),
    m_renderer(0),
    m_delegate(0),
    m_world(0),
    m_loader(0),
    m_debugDrawer(0),
    m_grid(0),
    m_info(0),
    m_bone(0),
    m_handles(0),
    m_settings(settings),
    m_editMode(kSelect),
    m_lastDistance(0.0f),
    m_prevElapsed(0.0f),
    m_frameIndex(0.0f),
    m_frameCount(0),
    m_currentFPS(0),
    m_interval(1000.0f / vpvl::Scene::kFPS),
    m_internalTimerID(0),
    m_handleFlags(0),
    m_playing(false),
    m_enableBoneMove(false),
    m_enableBoneRotate(false),
    m_showModelDialog(false),
    m_lockTouchEvent(false),
    m_enableMoveGesture(false),
    m_enableRotateGesture(false),
    m_enableScaleGesture(false),
    m_enableUndoGesture(false)
{
    m_debugDrawer = new DebugDrawer(this);
    m_delegate = new Delegate(this);
    m_grid = new Grid();
    m_info = new InfoPanel(this);
    m_world = new World(vpvl::Scene::kFPS);
    m_handles = new Handles(this);
    connect(static_cast<Application *>(qApp), SIGNAL(fileDidRequest(QString)), this, SLOT(loadFile(QString)));
    setShowModelDialog(m_settings->value("sceneWidget/showModelDialog", true).toBool());
    setMoveGestureEnable(m_settings->value("sceneWidget/enableMoveGesture", false).toBool());
    setRotateGestureEnable(m_settings->value("sceneWidget/enableRotateGesture", true).toBool());
    setScaleGestureEnable(m_settings->value("sceneWidget/enableScaleGesture", true).toBool());
    setUndoGestureEnable(m_settings->value("sceneWidget/enableUndoGesture", true).toBool());
    setAcceptDrops(true);
    setAutoFillBackground(false);
    setMinimumSize(540, 480);
    /* 通常はマウスを動かしても mouseMove が呼ばれないため、マウスが動いたら常時 mouseEvent を呼ぶようにする */
    setMouseTracking(true);
    grabGesture(Qt::PanGesture);
    grabGesture(Qt::PinchGesture);
    grabGesture(Qt::SwipeGesture);
}

SceneWidget::~SceneWidget()
{
    delete m_handles;
    m_handles = 0;
    delete m_info;
    m_info = 0;
    delete m_renderer;
    m_renderer = 0;
    delete m_grid;
    m_grid = 0;
    delete m_delegate;
    m_delegate = 0;
    delete m_world;
    m_world = 0;
}

SceneLoader *SceneWidget::sceneLoader() const
{
    return m_loader;
}

void SceneWidget::play()
{
    m_playing = true;
    m_timer.restart();
    emit sceneDidPlay();
}

void SceneWidget::pause()
{
    m_playing = false;
    m_info->setFPS(0);
    m_info->update();
    emit sceneDidPause();
}

void SceneWidget::stop()
{
    m_playing = false;
    m_info->setFPS(0);
    m_info->update();
    m_renderer->scene()->resetMotion();
    emit sceneDidStop();
}

void SceneWidget::clear()
{
    stop();
    setSelectedModel(0);
    m_loader->release();
    m_loader->createProject();
}

void SceneWidget::startAutomaticRendering()
{
    m_internalTimerID = startTimer(m_interval);
}

void SceneWidget::stopAutomaticRendering()
{
    killTimer(m_internalTimerID);
    m_internalTimerID = 0;
}

void SceneWidget::startPhysicsSimulation()
{
    /* 物理暴走を防ぐために少し進めてから開始する */
    if (m_loader && m_loader->isPhysicsEnabled()) {
        btDiscreteDynamicsWorld *world = m_world->mutableWorld();
        mutableScene()->setWorld(world);
        world->stepSimulation(1, 60);
    }
}

void SceneWidget::stopPhysicsSimulation()
{
    mutableScene()->setWorld(0);
    const vpvl::Array<vpvl::PMDModel *> &models = scene()->models();
    const int nmodels = models.count();
    for (int i = 0; i < nmodels; i++) {
        vpvl::PMDModel *model = models[i];
        model->resetAllBones();
    }
    updateMotion();
}

void SceneWidget::loadProject(const QString &filename)
{
    m_loader->loadProject(filename);
    m_world->setGravity(m_loader->worldGravity());
}

void SceneWidget::saveProject(const QString &filename)
{
    m_loader->setWorldGravity(m_world->gravity());
    m_loader->saveProject(filename);
}

const vpvl::Scene *SceneWidget::scene() const
{
    return m_renderer->scene();
}

vpvl::Scene *SceneWidget::mutableScene()
{
    return m_renderer->scene();
}

void SceneWidget::setPreferredFPS(int value)
{
    /* 一旦前のタイマーを止めてから新しい FPS に基づく間隔でタイマーを開始する */
    if (value > 0) {
        m_interval = 1000.0f / value;
        m_world->setPreferredFPS(value);
        m_renderer->scene()->setPreferredFPS(value);
        if (m_internalTimerID) {
            stopAutomaticRendering();
            startAutomaticRendering();
        }
    }
}

void SceneWidget::setSelectedModel(vpvl::PMDModel *value)
{
    /* 情報パネルに選択されたモデルの名前を更新する */
    m_loader->setSelectedModel(value);
    m_info->setModel(value);
    m_info->update();
}

void SceneWidget::setModelEdgeOffset(double value)
{
    if (vpvl::PMDModel *model = m_loader->selectedModel())
        m_loader->setModelEdgeOffset(model, static_cast<float>(value));
    updateMotion();
}

void SceneWidget::setModelEdgeColor(const QColor &color)
{
    if (vpvl::PMDModel *model = m_loader->selectedModel())
        m_loader->setModelEdgeColor(model, color);
    updateMotion();
}

void SceneWidget::setModelProjectiveShadowEnable(bool value)
{
    if (vpvl::PMDModel *model = m_loader->selectedModel())
        m_loader->setProjectiveShadowEnable(model, value);
    updateMotion();
}

void SceneWidget::setHandlesVisible(bool value)
{
    m_handles->setVisible(value);
}

void SceneWidget::setInfoPanelVisible(bool value)
{
    m_info->setVisible(value);
}

void SceneWidget::addModel()
{
    /* モデル追加と共に空のモーションを作成する */
    vpvl::PMDModel *model = addModel(openFileDialog("sceneWidget/lastPMDDirectory",
                                                    tr("Open PMD file"),
                                                    tr("PMD file (*.pmd)")));
    if (model && !m_playing) {
        setEmptyMotion(model);
        model->advanceMotion(0.0f);
        emit newMotionDidSet(model);
    }
}

vpvl::PMDModel *SceneWidget::addModel(const QString &path, bool skipDialog)
{
    QFileInfo fi(path);
    vpvl::PMDModel *model = 0;
    if (fi.exists()) {
        const QDir &dir = fi.dir();
        const QString &base = fi.fileName();
        model = m_loader->loadModel(base, dir);
        if (model) {
            if (skipDialog || (!m_showModelDialog || acceptAddingModel(model))) {
                ProgressDialogPtr progress = UIGetProgressDialog("Loading the model...", 0);
                QUuid uuid;
                m_loader->addModel(model, base, dir, uuid);
                progress.data()->setValue(1);
                emit fileDidLoad(path);
            }
            else {
                delete model;
                model = 0;
            }
        }
        else {
            QMessageBox::warning(this, tr("Loading model error"),
                                 tr("%1 cannot be loaded").arg(fi.fileName()));
        }
    }
    return model;
}

void SceneWidget::insertMotionToAllModels()
{
    /* モーションを追加したら即座に反映させるために advanceMotion(0.0f) を呼んでおく */
    vpvl::VMDMotion *motion = insertMotionToAllModels(openFileDialog("sceneWidget/lastVMDDirectory",
                                                                     tr("Open VMD (for model) file"),
                                                                     tr("VMD file (*.vmd)")));
    vpvl::PMDModel *selected = m_loader->selectedModel();
    if (motion && selected)
        selected->advanceMotion(0.0f);
}

vpvl::VMDMotion *SceneWidget::insertMotionToAllModels(const QString &path)
{
    vpvl::VMDMotion *motion = 0;
    if (QFile::exists(path)) {
        QList<vpvl::PMDModel *> models;
        motion = m_loader->loadModelMotion(path, models);
        if (motion) {
            emit fileDidLoad(path);
        }
        else {
            QMessageBox::warning(this, tr("Loading model motion error"),
                                 tr("%1 cannot be loaded").arg(QFileInfo(path).fileName()));
        }
    }
    return motion;
}

void SceneWidget::insertMotionToSelectedModel()
{
    vpvl::PMDModel *model = m_loader->selectedModel();
    if (model) {
        vpvl::VMDMotion *motion = insertMotionToSelectedModel(openFileDialog("sceneWidget/lastVMDDirectory",
                                                                             tr("Open VMD (for model) file"),
                                                                             tr("VMD file (*.vmd)")));
        if (motion)
            advanceMotion(0.0f);
    }
    else {
        QMessageBox::warning(this, tr("The model is not selected."),
                             tr("Select a model to insert the motion (\"Model\" > \"Select model\")"));
    }
}

vpvl::VMDMotion *SceneWidget::insertMotionToSelectedModel(const QString &path)
{
    return insertMotionToModel(path, m_loader->selectedModel());
}

vpvl::VMDMotion *SceneWidget::insertMotionToModel(const QString &path, vpvl::PMDModel *model)
{
    vpvl::VMDMotion *motion = 0;
    if (model) {
        if (QFile::exists(path)) {
            motion = m_loader->loadModelMotion(path, model);
            if (motion) {
                emit fileDidLoad(path);
            }
            else {
                QMessageBox::warning(this, tr("Loading model motion error"),
                                     tr("%1 cannot be loaded").arg(QFileInfo(path).fileName()));
            }
        }
    }
    return motion;
}

void SceneWidget::setEmptyMotion()
{
    setEmptyMotion(m_loader->selectedModel());
}

void SceneWidget::setEmptyMotion(vpvl::PMDModel *model)
{
    if (model) {
        vpvl::VMDMotion *modelMotion = m_loader->newModelMotion(model);
        m_loader->setModelMotion(modelMotion, model);
        vpvl::VMDMotion *cameraMotion = m_loader->newCameraMotion();
        m_loader->setCameraMotion(cameraMotion);
    }
    else
        QMessageBox::warning(this,
                             tr("The model is not selected."),
                             tr("Select a model to insert the motion (\"Model\" > \"Select model\")"));
}

void SceneWidget::addAsset()
{
    addAsset(openFileDialog("sceneWidget/lastAssetDirectory",
                            tr("Open X file"),
                            tr("DirectX mesh file (*.x)")));
}

vpvl::Asset *SceneWidget::addAsset(const QString &path)
{
    QFileInfo fi(path);
    vpvl::Asset *asset = 0;
    if (fi.exists()) {
        ProgressDialogPtr progress = UIGetProgressDialog("Loading the asset...", 0);
        QUuid uuid;
        asset = m_loader->loadAsset(fi.fileName(), fi.dir(), uuid);
        if (asset) {
            progress.data()->setValue(1);
            emit fileDidLoad(path);
        }
        else {
            QMessageBox::warning(this, tr("Loading asset error"),
                                 tr("%1 cannot be loaded").arg(fi.fileName()));
        }
    }
    return asset;
}


void SceneWidget::addAssetFromMetadata()
{
    addAssetFromMetadata(openFileDialog("sceneWidget/lastAssetDirectory",
                                        tr("Open VAC file"),
                                        tr("MMD accessory metadata (*.vac)")));
}

vpvl::Asset *SceneWidget::addAssetFromMetadata(const QString &path)
{
    QFileInfo fi(path);
    vpvl::Asset *asset = 0;
    if (fi.exists()) {
        QUuid uuid;
        ProgressDialogPtr progress = UIGetProgressDialog("Loading the asset...", 0);
        asset = m_loader->loadAssetFromMetadata(fi.fileName(), fi.dir(), uuid);
        if (asset) {
            progress.data()->setValue(1);
        }
        else {
            QMessageBox::warning(this, tr("Loading asset error"),
                                 tr("%1 cannot be loaded").arg(fi.fileName()));
        }
    }
    return asset;
}

void SceneWidget::saveMetadataFromAsset(vpvl::Asset *asset)
{
    if (asset) {
        QString filename = QFileDialog::getSaveFileName(this, tr("Save %1 as VAC file")
                                                        .arg(internal::toQString(asset)), "",
                                                        tr("MMD accessory metadata (*.vac)"));
        m_loader->saveMetadataFromAsset(filename, asset);
    }
}

void SceneWidget::insertPoseToSelectedModel()
{
    vpvl::PMDModel *model = m_loader->selectedModel();
    VPDFile *pose = insertPoseToSelectedModel(openFileDialog("sceneWidget/lastVPDDirectory",
                                                             tr("Open VPD file"),
                                                             tr("VPD file (*.vpd)")),
                                              model);
    if (pose && model)
        model->updateImmediate();
}

VPDFile *SceneWidget::insertPoseToSelectedModel(const QString &filename, vpvl::PMDModel *model)
{
    VPDFile *pose = 0;
    if (model) {
        if (QFile::exists(filename)) {
            pose = m_loader->loadModelPose(filename, model);
            if (!pose) {
                QMessageBox::warning(this, tr("Loading model pose error"),
                                     tr("%1 cannot be loaded").arg(QFileInfo(filename).fileName()));
            }
        }
    }
    else
        QMessageBox::warning(this,
                             tr("The model is not selected."),
                             tr("Select a model to set the pose (\"Model\" > \"Select model\")"));
    return pose;
}

void SceneWidget::advanceMotion(float frameIndex)
{
    vpvl::Scene *scene = m_renderer->scene();
    scene->updateModelView();
    scene->updateProjection();
    scene->advanceMotion(frameIndex);
    m_renderer->updateAllModel();
    updateGL();
    emit cameraPerspectiveDidSet(scene->cameraPosition(), scene->cameraAngle(), scene->fovy(), scene->cameraDistance());
}

void SceneWidget::seekMotion(float frameIndex, bool force)
{
    /* advanceMotion に似ているが、前のフレームインデックスを利用することがあるので、保存しておく必要がある */
    vpvl::Scene *scene = m_renderer->scene();
    scene->updateModelView();
    scene->updateProjection();
    /* 同じフレームインデックスにシークする場合はカメラと照明は動かさないようにする。force で強制的に動かすことが出来る */
    if (m_frameIndex == frameIndex && !force) {
        vpvl::VMDMotion *cameraMotion = scene->cameraMotion();
        vpvl::VMDMotion *lightMotion = scene->lightMotion();
        scene->setCameraMotion(0);
        scene->setLightMotion(0);
        scene->seekMotion(frameIndex);
        scene->setCameraMotion(cameraMotion);
        scene->setLightMotion(lightMotion);
    }
    else {
        scene->seekMotion(frameIndex);
        m_frameIndex = frameIndex;
    }
    m_renderer->updateAllModel();
    updateGL();
    emit cameraPerspectiveDidSet(scene->cameraPosition(), scene->cameraAngle(), scene->fovy(), scene->cameraDistance());
    emit motionDidSeek(frameIndex);
}

void SceneWidget::setCamera()
{
    vpvl::VMDMotion *motion = setCamera(openFileDialog("sceneWidget/lastCameraDirectory",
                                                       tr("Open VMD (for camera) file"),
                                                       tr("VMD file (*.vmd)")));
    if (motion)
        advanceMotion(0.0f);
}

vpvl::VMDMotion *SceneWidget::setCamera(const QString &path)
{
    vpvl::VMDMotion *motion = 0;
    if (QFile::exists(path)) {
        motion = m_loader->loadCameraMotion(path);
        if (motion) {
            emit fileDidLoad(path);
        }
        else {
            QMessageBox::warning(this, tr("Loading camera motion error"),
                                 tr("%1 cannot be loaded").arg(QFileInfo(path).fileName()));
        }
    }
    return motion;
}

void SceneWidget::deleteSelectedModel()
{
    vpvl::PMDModel *selected = m_loader->selectedModel();
    if (selected) {
        QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this,
                                   qAppName(),
                                   tr("Do you want to delete the model \"%1\"? This cannot undo.")
                                   .arg(internal::toQString(selected)),
                                   QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::Yes)
            m_loader->deleteModel(selected);
    }
}

void SceneWidget::resetCamera()
{
    vpvl::Scene *scene = m_renderer->scene();
    scene->resetCamera();
    emit cameraPerspectiveDidSet(scene->cameraPosition(), scene->cameraAngle(), scene->fovy(), scene->cameraDistance());
}

void SceneWidget::setLightColor(const vpvl::Color &color)
{
    vpvl::Scene *scene = m_renderer->scene();
    scene->setLightSource(color, scene->lightPosition());
    emit lightColorDidSet(color);
}

void SceneWidget::setLightPosition(const vpvl::Vector3 &position)
{
    vpvl::Scene *scene = m_renderer->scene();
    scene->setLightSource(scene->lightColor(), position);
    emit lightPositionDidSet(position);
}

void SceneWidget::setCameraPerspective(vpvl::Vector3 *pos, vpvl::Vector3 *angle, float *fovy, float *distance)
{
    /* 変更しないことを示す NULL かどうかを判定するために引数をポインタに設定している */
    vpvl::Scene *scene = m_renderer->scene();
    vpvl::Vector3 posValue, angleValue;
    float fovyValue, distanceValue;
    posValue = !pos ? scene->cameraPosition() : *pos;
    angleValue = !angle ? scene->cameraAngle() : *angle;
    fovyValue = !fovy ? scene->fovy() : *fovy;
    distanceValue = !distance ? scene->cameraDistance() : *distance;
    scene->setCameraPerspective(posValue, angleValue, fovyValue, distanceValue);
    emit cameraPerspectiveDidSet(posValue, angleValue, fovyValue, distanceValue);
}

void SceneWidget::makeRay(const QPointF &input, vpvl::Vector3 &rayFrom, vpvl::Vector3 &rayTo) const
{
    // This implementation based on the below page.
    // http://softwareprodigy.blogspot.com/2009/08/gluunproject-for-iphone-opengl-es.html
    vpvl::Scene *scene = m_renderer->scene();
    float modelviewMatrixf[16], projectionMatrixf[16];
    GLdouble modelviewMatrixd[16], projectionMatrixd[16];
    const GLint viewport[4] = { 0, 0, width(), height() };
    scene->getModelViewMatrix(modelviewMatrixf);
    scene->getProjectionMatrix(projectionMatrixf);
    for (int i = 0; i < 16; i++) {
        modelviewMatrixd[i] = modelviewMatrixf[i];
        projectionMatrixd[i] = projectionMatrixf[i];
    }
    GLdouble wx = input.x(), wy = height() - input.y(), cx, cy, cz, fx, fy, fz;
    gluUnProject(wx, wy, 0, modelviewMatrixd, projectionMatrixd, viewport, &cx, &cy, &cz);
    gluUnProject(wx, wy, 1, modelviewMatrixd, projectionMatrixd, viewport, &fx, &fy, &fz);
    rayFrom.setValue(cx, cy, cz);
    rayTo.setValue(fx, fy, fz);
}

void SceneWidget::selectBones(const QList<vpvl::Bone *> &bones)
{
    m_info->setBones(bones, tr("(multiple)"));
    m_info->update();
    if (!bones.isEmpty()) {
        vpvl::Bone *bone = bones.first();
        m_handles->setMovable(bone->isMovable());
        m_handles->setRotateable(bone->isRotateable());
        m_handles->setBone(bone);
        m_bone = bone;
    }
    else {
        m_handles->setMovable(false);
        m_handles->setRotateable(false);
        m_handles->setBone(0);
        m_bone = 0;
    }
}

void SceneWidget::rotateScene(const vpvl::Vector3 &delta)
{
    vpvl::Scene *scene = m_renderer->scene();
    vpvl::Vector3 pos = scene->cameraPosition(), angle = scene->cameraAngle();
    float fovy = scene->fovy(), distance = scene->cameraDistance();
    angle += delta;
    scene->setCameraPerspective(pos, angle, fovy, distance);
    emit cameraPerspectiveDidSet(pos, angle, fovy, distance);
}

void SceneWidget::rotateModel(const vpvl::Quaternion &delta)
{
    rotateModel(m_loader->selectedModel(), delta);
}

void SceneWidget::rotateModel(vpvl::PMDModel *model, const vpvl::Quaternion &delta)
{
    if (model) {
        const vpvl::Quaternion &rotation = model->rotationOffset();
        model->setRotationOffset(rotation * delta);
        model->updateImmediate();
        emit modelDidRotate(rotation);
    }
}

void SceneWidget::translateScene(const vpvl::Vector3 &delta)
{
    // FIXME: direction
    vpvl::Scene *scene = m_renderer->scene();
    vpvl::Vector3 pos = scene->cameraPosition(), angle = scene->cameraAngle();
    float fovy = scene->fovy(), distance = scene->cameraDistance();
    pos += delta;
    scene->setCameraPerspective(pos, angle, fovy, distance);
    emit cameraPerspectiveDidSet(pos, angle, fovy, distance);
}

void SceneWidget::translateModel(const vpvl::Vector3 &delta)
{
    translateModel(m_loader->selectedModel(), delta);
}

void SceneWidget::translateModel(vpvl::PMDModel *model, const vpvl::Vector3 &delta)
{
    // FIXME: direction
    if (model) {
        const vpvl::Vector3 &position = model->positionOffset();
        model->setPositionOffset(position + delta);
        model->updateImmediate();
        emit modelDidMove(position);
    }
}

void SceneWidget::resetModelPosition()
{
    vpvl::PMDModel *model = m_loader->selectedModel();
    if (model) {
        const vpvl::Vector3 &position = model->positionOffset();
        model->setPositionOffset(vpvl::Vector3(0.0f, 0.0f, 0.0f));
        model->updateImmediate();
        emit modelDidMove(position);
    }
}

void SceneWidget::loadFile(const QString &file)
{
    /* モデルファイル */
    if (file.endsWith(".pmd", Qt::CaseInsensitive)) {
        vpvl::PMDModel *model = addModel(file);
        if (model && !m_playing) {
            setEmptyMotion(model);
            model->advanceMotion(0.0f);
            emit newMotionDidSet(model);
        }
    }
    /* モーションファイル */
    else if (file.endsWith(".vmd", Qt::CaseInsensitive)) {
        vpvl::VMDMotion *motion = insertMotionToModel(file, m_loader->selectedModel());
        if (motion)
            advanceMotion(0.0f);
    }
    /* アクセサリファイル */
    else if (file.endsWith(".x", Qt::CaseInsensitive)) {
        addAsset(file);
    }
    /* ポーズファイル */
    else if (file.endsWith(".vpd", Qt::CaseInsensitive)) {
        vpvl::PMDModel *model = m_loader->selectedModel();
        VPDFile *pose = insertPoseToSelectedModel(file, model);
        if (pose && model)
            model->updateImmediate();
    }
    /* アクセサリ情報ファイル */
    else if (file.endsWith(".vac", Qt::CaseInsensitive)) {
        addAssetFromMetadata(file);
    }
}

void SceneWidget::zoom(bool up, const Qt::KeyboardModifiers &modifiers)
{
    vpvl::Scene *scene = m_renderer->scene();
    vpvl::Vector3 pos = scene->cameraPosition(), angle = scene->cameraAngle();
    float fovy = scene->fovy(), distance = scene->cameraDistance();
    float fovyStep = 1.0f, distanceStep = 4.0f;
    if (modifiers & Qt::ControlModifier && modifiers & Qt::ShiftModifier) {
        fovy = up ? fovy - fovyStep : fovy + fovyStep;
    }
    else {
        if (modifiers & Qt::ControlModifier)
            distanceStep *= 5.0f;
        else if (modifiers & Qt::ShiftModifier)
            distanceStep *= 0.2f;
        if (distanceStep != 0.0f)
            distance = up ? distance - distanceStep : distance + distanceStep;
    }
    scene->setCameraPerspective(pos, angle, fovy, distance);
    emit cameraPerspectiveDidSet(pos, angle, fovy, distance);
}

bool SceneWidget::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture && !m_lockTouchEvent)
        return gestureEvent(static_cast<QGestureEvent *>(event));
    return QGLWidget::event(event);
}

void SceneWidget::closeEvent(QCloseEvent *event)
{
    m_settings->setValue("sceneWidget/showModelDialog", showModelDialog());
    m_settings->setValue("sceneWidget/enableMoveGesture", isMoveGestureEnabled());
    m_settings->setValue("sceneWidget/enableRotateGesture", isRotateGestureEnabled());
    m_settings->setValue("sceneWidget/enableScaleGesture", isScaleGestureEnabled());
    m_settings->setValue("sceneWidget/enableUndoGesture", isUndoGestureEnabled());
    stopAutomaticRendering();
    event->accept();
}

void SceneWidget::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void SceneWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
}

void SceneWidget::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

void SceneWidget::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        const QList<QUrl> &urls = mimeData->urls();
        foreach (const QUrl url, urls) {
            const QString &file = url.toLocalFile();
            loadFile(file);
            qDebug() << "Proceeded a dropped file:" << file;
        }
    }
}

void SceneWidget::initializeGL()
{
    initializeGLFunctions(context());
    qDebug("VPVL version: %s (%d)", VPVL_VERSION_STRING, VPVL_VERSION);
    qDebug("GL_VERSION: %s", glGetString(GL_VERSION));
    qDebug("GL_VENDOR: %s", glGetString(GL_VENDOR));
    qDebug("GL_RENDERER: %s", glGetString(GL_RENDERER));
    EnableMultisample();
    /* OpenGL の初期化が最低条件なため、Renderer はここでインスタンスを作成する */
    m_renderer = new Renderer(m_delegate, width(), height(), vpvl::Scene::kFPS);
    m_loader = new SceneLoader(m_renderer);
    m_debugDrawer->setWorld(m_world->mutableWorld());
    /* OpenGL を利用するため、格子状フィールドの初期化もここで行う */
    m_grid->load();
    vpvl::Scene *scene = m_renderer->scene();
    /* 物理演算に必要な World が initializeGL でインスタンスを生成するため、setPhysicsEnable はここで有効にする */
    //if (m_playing || m_enablePhysics)
    //    setPhysicsEnable(true);
    /* テクスチャ情報を必要とするため、ハンドルのリソースの読み込みはここで行う */
    m_handles->load();
    /* 動的なテクスチャ作成を行うため、情報パネルのリソースの読み込みも個々で行った上で初期設定を行う */
    m_info->load();
    m_info->setModel(0);
    m_info->setBones(QList<vpvl::Bone *>(), "");
    m_info->setFPS(0.0f);
    m_info->update();
    m_debugDrawer->initialize();
    m_renderer->initializeSurface();
    m_timer.start();
    startAutomaticRendering();
    emit cameraPerspectiveDidSet(scene->cameraPosition(), scene->cameraAngle(), scene->fovy(), scene->cameraDistance());
    emit initailizeGLContextDidDone();
}

void SceneWidget::mousePressEvent(QMouseEvent *event)
{
    const QPointF &pos = event->posF();
    QRectF rect;
    int flags;
    m_lockTouchEvent = true;
    m_handles->setPoint2D(pos);
    /* モデルのハンドルと重なるケースを考慮して右下のハンドルを優先的に処理する */
    if (m_handles->testHitImage(pos, flags, rect)) {
        switch (flags) {
        /*
         * ローカルとグローバルの切り替えなので、値を反転して設定する必要がある
         * また、ローカルとグローバル、移動回転ハンドルはそれぞれフラグ値は排他的
         */
        case Handles::kLocal:
            m_handles->setLocal(false);
            break;
        case Handles::kGlobal:
            m_handles->setLocal(true);
            break;
        default:
            setCursor(Qt::SizeVerCursor);
            break;
        }
        m_handleFlags = flags;
        emit handleDidGrab();
        return;
    }
    if (vpvl::PMDModel *model = m_loader->selectedModel()) {
        if (m_editMode == kSelect) {
            static const vpvl::Vector3 size(0.1f, 0.1f, 0.1f);
            const QPointF &pos = event->posF();
            vpvl::Vector3 znear, zfar, normal;
            makeRay(pos, znear, zfar);
            const vpvl::BoneList &bones = model->bones();
            const int nbones = bones.count();
            vpvl::Bone *nearestBone = 0;
            vpvl::Scalar hitLambda = 1.0f;
            for (int i = 0; i < nbones; i++) {
                vpvl::Bone *bone = bones[i];
                const vpvl::Vector3 &o = bone->localTransform().getOrigin(),
                        min = o - size, max = o + size;
                if (btRayAabb(znear, zfar, min, max, hitLambda, normal)) {
                    nearestBone = bone;
                    break;
                }
            }
            if (nearestBone && (nearestBone->isMovable() || nearestBone->isRotateable())) {
                QList<vpvl::Bone *> bones;
                bones.append(nearestBone);
                emit boneDidSelect(bones);
            }
        }
        else if (m_editMode == kRotate) { /* only in move or rotate mode */
            vpvl::Vector3 rayFrom, rayTo, pick;
            makeRay(pos, rayFrom, rayTo);
            if (m_handles->testHitModel(rayFrom, rayTo, false, flags, pick)) {
                m_handleFlags = flags;
                m_handles->setVisibilityFlags(flags);
                setCursor(Qt::ClosedHandCursor);
                emit handleDidGrab();
            }
        }
    }
}

void SceneWidget::mouseMoveEvent(QMouseEvent *event)
{
    const QPointF &pos = event->posF();
    if (event->buttons() & Qt::LeftButton) {
        const Qt::KeyboardModifiers modifiers = event->modifiers();
        const QPointF &diff = m_handles->diffPoint2D(pos);
        /* モデルのハンドルがクリックされた */
        if (m_handleFlags & Handles::kView)
            grabModelHandleByRaycast(pos);
        /* 有効な右下のハンドルがクリックされた */
        else if (m_handleFlags & Handles::kEnable)
            grabImageHandle(diff);
        /* 光源移動 */
        else if (modifiers & Qt::ControlModifier && modifiers & Qt::ShiftModifier) {
            vpvl::Scene *scene = m_renderer->scene();
            vpvl::Vector3 position = scene->lightPosition();
            vpvl::Quaternion rx(0.0f, diff.y() * vpvl::radian(0.1f), 0.0f),
                    ry(0.0f, diff.x() * vpvl::radian(0.1f), 0.0f);
            position = position * btMatrix3x3(rx * ry);
            scene->setLightSource(scene->lightColor(), position);
        }
        /* 場面の移動 */
        else if (modifiers & Qt::ShiftModifier) {
            translateScene(vpvl::Vector3(diff.x() * -0.1f, diff.y() * 0.1f, 0.0f));
        }
        /* 場面の回転 (X と Y が逆転している点に注意) */
        else {
            rotateScene(vpvl::Vector3(diff.y() * 0.5f, diff.x() * 0.5f, 0.0f));
        }
        m_handles->setPoint2D(event->posF());
        return;
    }
    changeCursorIfHandlesHit(pos);
}

void SceneWidget::mouseReleaseEvent(QMouseEvent *event)
{
    changeCursorIfHandlesHit(event->posF());
    m_handles->setVisibilityFlags(Handles::kVisibleAll);
    m_handleFlags = Handles::kNone;
    m_handles->setAngle(0.0f);
    m_handles->setPoint3D(vpvl::Vector3(0.0f, 0.0f, 0.0f));
    m_lockTouchEvent = false;
    emit handleDidRelease();
}

void SceneWidget::paintGL()
{
    qglClearColor(m_loader->isBlackBackgroundEnabled() ? Qt::black : Qt::white);
    EnableMultisample();
    m_renderer->clear();
    {
        size_t size = 0;
        vpvl::PMDModel **models = m_renderer->scene()->getRenderingOrder(size);
        glCullFace(GL_FRONT);
        for (size_t i = 0; i < size; i++) {
            vpvl::PMDModel *model = models[i];
            if (m_loader->isProjectiveShadowEnabled(model))
                m_renderer->renderModelShadow(model);
        }
        glCullFace(GL_BACK);
    }
    m_renderer->renderAllModels();
    m_renderer->renderAllAssets();
    m_grid->draw(m_renderer->scene(), m_loader->isGridVisible());
    if (m_editMode == kSelect)
        m_debugDrawer->drawModelBones(m_loader->selectedModel(), true, true);
    m_handles->drawImageHandles();
    if (m_editMode == kRotate)
        m_handles->drawModelHandles();
    m_debugDrawer->drawBoneTransform(selectedBone());
    m_info->draw();
}

void SceneWidget::resizeGL(int w, int h)
{
    m_renderer->resize(w, h);
    m_handles->resize(w, h);
    m_info->resize(w, h);
}

void SceneWidget::timerEvent(QTimerEvent *event)
{
    /* タイマーが生きている => 描写命令を出す */
    if (event->timerId() == m_internalTimerID) {
        vpvl::Scene *scene = m_renderer->scene();
        scene->updateModelView();
        scene->updateProjection();
        m_renderer->updateAllModel();
        if (m_playing) {
            /* タイマーの仕様上一定ではないため、差分をここで吸収する */
            float elapsed = m_timer.elapsed() / static_cast<float>(vpvl::Scene::kFPS);
            float diff = elapsed - m_prevElapsed;
            m_prevElapsed = elapsed;
            if (diff < 0)
                diff = elapsed;
            scene->advanceMotion(diff);
            updateFPS();
        }
        updateGL();
    }
}

void SceneWidget::wheelEvent(QWheelEvent *event)
{
    zoom(event->delta() > 0, event->modifiers());
}

bool SceneWidget::gestureEvent(QGestureEvent *event)
{
    if (QGesture *swipe = event->gesture(Qt::SwipeGesture))
        swipeTriggered(static_cast<QSwipeGesture *>(swipe));
    else if (QGesture *pan = event->gesture(Qt::PanGesture))
        panTriggered(static_cast<QPanGesture *>(pan));
    if (QGesture *pinch = event->gesture(Qt::PinchGesture))
        pinchTriggered(static_cast<QPinchGesture *>(pinch));
    return true;
}

void SceneWidget::panTriggered(QPanGesture *event)
{
    if (!m_enableMoveGesture)
        return;
    const Qt::GestureState state = event->state();
    switch (state) {
    case Qt::GestureStarted:
    case Qt::GestureUpdated:
        setCursor(Qt::SizeAllCursor);
        break;
    default:
        setCursor(Qt::ArrowCursor);
        break;
    }
    const QPointF &delta = event->delta();
    const vpvl::Vector3 newDelta(delta.x() * -0.25, delta.y() * 0.25, 0.0f);
    if (vpvl::Bone *bone = selectedBone()) {
        switch (state) {
        case Qt::GestureStarted:
            emit handleDidGrab();
            break;
        case Qt::GestureUpdated:
            emit handleDidMove(newDelta, bone, 'V');
            break;
        case Qt::GestureFinished:
            emit handleDidRelease();
            break;
        default:
            break;
        }
    }
    else if (vpvl::PMDModel *model = m_loader->selectedModel())
        translateModel(model, newDelta);
    else
        translateScene(newDelta);
}

void SceneWidget::pinchTriggered(QPinchGesture *event)
{
    const Qt::GestureState state = event->state();
    QPinchGesture::ChangeFlags flags = event->changeFlags();
    vpvl::Scene *scene = m_renderer->scene();
    const vpvl::Vector3 &pos = scene->cameraPosition(), &angle = scene->cameraAngle();
    float distance = scene->cameraDistance(), fovy = scene->fovy();
    if (m_enableRotateGesture && flags & QPinchGesture::RotationAngleChanged) {
        qreal value = event->rotationAngle() - event->lastRotationAngle();
        vpvl::Scalar radian = vpvl::radian(value);
        vpvl::Quaternion rotation(0.0f, 0.0f, 0.0f, 1.0f);
        /* 四元数を使う場合回転が時計回りになるよう符号を反転させる必要がある */
        if (vpvl::Bone *bone = selectedBone()) {
            rotation.setEulerZYX(0.0f, -radian, 0.0f);
            switch (state) {
            case Qt::GestureStarted:
                emit handleDidGrab();
                break;
            case Qt::GestureUpdated:
                emit handleDidRotate(rotation, bone, 'V', float(value));
                break;
            case Qt::GestureFinished:
                emit handleDidRelease();
                break;
            default:
                break;
            }
        }
        else if (vpvl::PMDModel *model = m_loader->selectedModel()) {
            rotation.setEulerZYX(0.0f, -radian, 0.0f);
            rotateModel(model, rotation);
        }
        else {
            rotateScene(vpvl::Vector3(0.0f, value, 0.0f));
        }
    }
    qreal scaleFactor = 1.0;
    if (state == Qt::GestureStarted)
        m_lastDistance = distance;
    if (m_enableScaleGesture && flags & QPinchGesture::ScaleFactorChanged) {
        scaleFactor = event->scaleFactor();
        distance = m_lastDistance * scaleFactor;
    }
    if (scaleFactor != 1.0) {
        scene->setCameraPerspective(pos, angle, fovy, distance);
        emit cameraPerspectiveDidSet(pos, angle, fovy, distance);
    }
}

void SceneWidget::swipeTriggered(QSwipeGesture *event)
{
    if (m_enableUndoGesture && event->state() == Qt::GestureFinished) {
        const QSwipeGesture::SwipeDirection hdir = event->horizontalDirection();
        const QSwipeGesture::SwipeDirection vdir = event->verticalDirection();
        if (hdir == QSwipeGesture::Left || vdir == QSwipeGesture::Up) {
            emit undoDidRequest();
        }
        else if (hdir == QSwipeGesture::Right || vdir == QSwipeGesture::Down) {
            emit redoDidRequest();
        }
    }
}

bool SceneWidget::acceptAddingModel(vpvl::PMDModel *model)
{
    /* モデルを追加する前にモデルの名前とコメントを出すダイアログを表示 */
    QMessageBox mbox;
    QString comment = internal::toQString(model->comment());
    mbox.setText(tr("Model Information of \"%1\"").arg(internal::toQString(model->name())));
    mbox.setInformativeText(comment.replace("\n", "<br>"));
    mbox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    return mbox.exec() == QMessageBox::Ok;
}

void SceneWidget::updateFPS()
{
    /* 1秒ごとの FPS はここで計算しておく。1秒過ぎたら updateFPS を呼んだ回数を求め、タイマーを再起動させる */
    if (m_timer.elapsed() > 1000) {
        m_currentFPS = m_frameCount;
        m_frameCount = 0;
        m_timer.restart();
        m_info->setFPS(m_currentFPS);
        m_info->update();
        emit fpsDidUpdate(m_currentFPS);
    }
    m_frameCount++;
}

const QString SceneWidget::openFileDialog(const QString &name, const QString &desc, const QString &exts)
{
    /* ファイルが選択されている場合はファイルが格納されているディレクトリを指す絶対パスを設定に保存しておく */
    const QString &path = m_settings->value(name).toString();
    const QString &fileName = QFileDialog::getOpenFileName(this, desc, path, exts);
    if (!fileName.isEmpty()) {
        QDir dir(fileName);
        dir.cdUp();
        m_settings->setValue(name, dir.absolutePath());
    }
    return fileName;
}

void SceneWidget::changeCursorIfHandlesHit(const QPointF &pos)
{
    int flags;
    QRectF rect;
    /* ハンドルアイコンの中に入っているか? */
    if (m_handles->testHitImage(pos, flags, rect) && flags & Handles::kEnable) {
        setCursor(Qt::SizeVerCursor);
    }
    /* 回転モードの場合は回転ハンドルに入っているか? */
    else if (m_editMode == kRotate) {
        vpvl::Vector3 rayFrom, rayTo, pick;
        makeRay(pos, rayFrom, rayTo);
        if (m_handles->testHitModel(rayFrom, rayTo, true, flags, pick))
            setCursor(Qt::OpenHandCursor);
        else
            unsetCursor();
    }
    else {
        unsetCursor();
    }
}

void SceneWidget::grabImageHandle(const QPointF &diff)
{
    int flags = m_handleFlags;
    int mode = m_handles->isLocal() ? 'L' : 'G';
    /* 意図する向きと実際の値が逆なので、反転させる */
    float diffValue = -diff.y();
    /* 移動ハンドルである */
    if (flags & Handles::kMove) {
        const float &value = diffValue * 0.1f;
        vpvl::Vector3 delta(0.0f, 0.0f, 0.0f);
        if (flags & Handles::kX)
            delta.setX(value);
        else if (flags & Handles::kY)
            delta.setY(value);
        else if (flags & Handles::kZ)
            delta.setZ(value);
        emit handleDidMove(delta, 0, mode);
    }
    /* 回転ハンドルである */
    else if (flags & Handles::kRotate) {
        const float &value = vpvl::radian(diffValue) * 0.1f;
        vpvl::Quaternion delta(0.0f, 0.0f, 0.0f, 1.0f);
        int axis = 0;
        if (flags & Handles::kX) {
            delta.setX(value);
            axis = 'X' << 8;
        }
        else if (flags & Handles::kY) {
            delta.setY(value);
            axis = 'Y' << 8;
        }
        else if (flags & Handles::kZ) {
            delta.setZ(-value);
            axis = 'Z' << 8;
        }
        emit handleDidRotate(delta, 0, mode | axis, diffValue);
    }
}

void SceneWidget::grabModelHandleByRaycast(const QPointF &pos)
{
#if 1
    int flags, mode = 'V';
    vpvl::Vector3 rayFrom, rayTo, pick;
    makeRay(pos, rayFrom, rayTo);
    /* モデルのハンドルに当たっている場合のみモデルを動かす */
    if (m_handles->testHitModel(rayFrom, rayTo, false, flags, pick)) {
        const vpvl::Vector3 directionX(-1.0f, 0.0f, 0.0f),
                directionY(0.0f, -1.0f, 0.0f),
                directionZ(0.0f, 0.0f, 1.0f),
                &diff = m_handles->diffPoint3D(pick);
        mode = 'L';
        /* 移動ハンドルである(矢印の先端) */
        if (flags & Handles::kMove && !diff.isZero()) {
            float value = m_handles->isPoint3DZero() ? 0.0f : diff.length();
            vpvl::Vector3 delta(0.0f, 0.0f, 0.0f);
            if (flags & Handles::kX) {
                delta.setX(value);
            }
            else if (flags & Handles::kY) {
                delta.setY(value);
            }
            else if (flags & Handles::kZ) {
                delta.setZ(value);
            }
            emit handleDidMove(delta, 0, mode);
        }
        /* 回転ハンドルである(ドーナツ) */
        else if (flags & Handles::kRotate) {
            const btScalar &angle = m_handles->angle(pick);
            vpvl::Quaternion delta(0.0f, 0.0f, 0.0f, 1.0f);
            if (!m_handles->isAngleZero()) {
                float diff = m_handles->diffAngle(angle);
                if (flags & Handles::kX)
                    delta.setEulerZYX(0.0f, 0.0f, btFabs(diff));
                else if (flags & Handles::kY)
                    delta.setEulerZYX(0.0f, btFabs(diff), 0.0f);
                else if (flags & Handles::kZ)
                    delta.setEulerZYX(btFabs(diff), 0.0f, 0.0f);
                emit handleDidRotate(delta, 0, mode, diff);
            }
            m_handles->setAngle(angle);
        }
        m_handles->setPoint3D(pick);
    }
#else
    int mode = 'V';
    const QPointF &diff = m_handles->diffPoint2D(pos);
    if (m_handleFlags & Handles::kMove && !diff.isNull()) {
        vpvl::Vector3 delta(0.0f, 0.0f, 0.0f);
        if (m_handleFlags & Handles::kX) {
            delta.setX(diff.x() * 0.1);
        }
        else if (m_handleFlags & Handles::kY) {
            delta.setY(diff.y() * 0.1);
        }
        else if (m_handleFlags & Handles::kZ) {
            delta.setZ(diff.y() * 0.1);
        }
        emit handleDidMove(delta, 0, mode);
    }
#endif
}

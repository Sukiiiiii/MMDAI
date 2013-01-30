QT += core gui opengl
greaterThan(QT_MAJOR_VERSION, 4):QT += widgets concurrent

TARGET = MMDAI2
TEMPLATE = app
DEFINES += IS_VPVM
macx:DEFINES += USE_FILE32API

# libvpvl2 and base libraries
ASSIMP_PATH = ../assimp-src
BULLET_PATH = ../bullet-src
VPVL1_PATH = ../libvpvl
VPVL2_PATH = ../libvpvl2
MMDA_PATH = ../../MMDAgent/MMDAgent
LIBAV_PATH = ../libav-src
NVTT_PATH = ../nvtt-src
GLEW_PATH = ../glew-src
GLM_PATH = ../glm-src
PORTAUDIO_PATH = ../portaudio-src
ICU_PATH = ../icu-src

# CMake prefix path (mainly for win32)
exists($$(CMAKE_PREFIX_PATH)/include):INCLUDEPATH += "$$(CMAKE_PREFIX_PATH)/include"
exists($$(CMAKE_PREFIX_PATH)/lib):LIBS += -L"$$(CMAKE_PREFIX_PATH)/lib"

# libxml2
exists(/usr/include/libxml2):INCLUDEPATH += /usr/include/libxml2
exists(/usr/local/include/libxml2):INCLUDEPATH += /usr/local/include/libxml2

# configuration by build type
CONFIG(debug, debug|release) {
  greaterThan(QT_MAJOR_VERSION, 4):BUILD_DIRECTORY_VPVL2_SUFFIX  = -qt5
  BUILD_TYPE = debug
  # should not change link order because of static library link order
}
CONFIG(release, debug|release) {
  greaterThan(QT_MAJOR_VERSION, 4):BUILD_DIRECTORY_VPVL2_SUFFIX  = -qt5
  BUILD_TYPE = release
  # should not change link order because of static library link order
  LIBS      += -lvpvl2qtcommon -lvpvl2 -lvpvl
}
BUILD_DIRECTORY = build-$${BUILD_TYPE}
BUILD_DIRECTORY_WITH_NATIVE_SUFFIX = $${BUILD_DIRECTORY}-native
VPVL2_BUILD_DIRECTORY = $${BUILD_DIRECTORY}$${BUILD_DIRECTORY_VPVL2_SUFFIX}
win32 {
  LIBRARY_DIRECTORY = $${BUILD_DIRECTORY}/lib/$${BUILD_TYPE}
  ASSIMP_LIBRARY_DIRECTORY = $${BUILD_DIRECTORY}/code/$${BUILD_TYPE}
  VPVL2_LIBRARY_DIRECTORY = $${VPVL2_BUILD_DIRECTORY}/lib/$${BUILD_TYPE}
  LIBS += -L$${NVTT_PATH}/$${BUILD_DIRECTORY}/src/nvcore/$${BUILD_TYPE} \
          -L$${NVTT_PATH}/$${BUILD_DIRECTORY}/src/nvimage/$${BUILD_TYPE} \
          -L$${NVTT_PATH}/$${BUILD_DIRECTORY}/src/nvmath/$${BUILD_TYPE} \
          -L$$(TBB_INSTALL_DIR)/lib/$$(TBB_ARCH_PLATFORM)
}
!win32 {
  LIBRARY_DIRECTORY = $${BUILD_DIRECTORY}/lib
  ASSIMP_LIBRARY_DIRECTORY = $${BUILD_DIRECTORY}/lib
  CONFIG(debug, debug|release):VPVL2_LIBRARY_SUFFIX = _debug
  VPVL2_LIBRARY_DIRECTORY = $${BUILD_DIRECTORY}$${BUILD_DIRECTORY_VPVL2_SUFFIX}/lib
  LIBS += -L$${NVTT_PATH}/$${LIBRARY_DIRECTORY}
}

# VPVL and others configuration
LIBS             += -L$${ASSIMP_PATH}/$${ASSIMP_LIBRARY_DIRECTORY} \
                    -L$${BULLET_PATH}/$${LIBRARY_DIRECTORY} \
                    -L$${ICU_PATH}/$${LIBRARY_DIRECTORY} \
                    -L$${VPVL1_PATH}/$${LIBRARY_DIRECTORY} \
                    -L$${VPVL2_PATH}/$${VPVL2_LIBRARY_DIRECTORY} \
                    -L$${GLEW_PATH}/lib
INCLUDEPATH      += $${VPVL2_PATH}/include \
                    $${VPVL2_PATH}/$${VPVL2_BUILD_DIRECTORY}/include \
                    $${ASSIMP_PATH}/include \
                    $${PORTAUDIO_PATH}/include \
                    $${BULLET_PATH}/src \
                    $${NVTT_PATH}/$${BUILD_DIRECTORY}/src \
                    $${NVTT_PATH}/extern/poshlib \
                    $${NVTT_PATH}/src \
                    $${GLM_PATH} \
                    $${ICU_PATH}/source/common \
                    $${ICU_PATH}/source/i18n \
                    $${LIBAV_PATH}/$${BUILD_DIRECTORY_WITH_NATIVE_SUFFIX}/include \
                    $${PORTAUDIO_PATH}/$${BUILD_DIRECTORY_WITH_NATIVE_SUFFIX}/include

# Required libraries
LIBS += -lvpvl2qtcommon$${VPVL2_LIBRARY_SUFFIX} \
        -lvpvl2$${VPVL2_LIBRARY_SUFFIX} \
        -lvpvl$${VPVL2_LIBRARY_SUFFIX} \
        -lassimp \
        -lBulletSoftBody \
        -lBulletDynamics \
        -lBulletCollision \
        -lLinearMath \
        -lnvimage \
        -lnvmath \
        -lnvcore \
        -ltbb

win32 {
  LIBS += -llibxml2_a \
          -lws2_32 \
          -liconv \
          -licui18n \
          -licuuc \
          -licudata \
          -lglew32s \
          -lz
}
!win32 {
  LIBS += -lxml2 \
          -lGLEW \
          -licui18n \
          -licuuc \
          -licudata
}

macx:LIBS += -framework OpenCL \
             -framework CoreServices \
             -framework Cg \
             -framework CoreAudio \
             -framework AudioToolbox \
             -framework AudioUnit
linux-*:LIBS += -lCg -lCgGL

# based on QtCreator's qmake spec
DEFINES += QT_NO_CAST_TO_ASCII
contains(QT_CONFIG, reduce_exports):CONFIG += hide_symbols
unix {
  CONFIG(debug, debug|release):OBJECTS_DIR = $${OUT_PWD}/.obj/debug-shared
  CONFIG(release, debug|release):OBJECTS_DIR = $${OUT_PWD}/.obj/release-shared
  CONFIG(debug, debug|release):MOC_DIR = $${OUT_PWD}/.moc/debug-shared
  CONFIG(release, debug|release):MOC_DIR = $${OUT_PWD}/.moc/release-shared
  RCC_DIR = $${OUT_PWD}/.rcc
  UI_DIR = $${OUT_PWD}/.uic
}

translations.files = resources/translations/VPVM_ja.qm \
                     $$[QT_INSTALL_TRANSLATIONS]/qt_ja.qm
win32 {
  NVIDIA_CG_PATH  = "C:/Program Files (x86)/NVIDIA Corporation/Cg"
  LIBS           += -L$${NVIDIA_CG_PATH}/lib -lcg -lcggl
  INCLUDEPATH    += $${PORTAUDIO_PATH}/build-debug-native/include \
                    $${NVIDIA_CG_PATH}/include
  QMAKE_CFLAGS   += /wd4250 /wd4251 /wd4819
  QMAKE_CXXFLAGS += /wd4250 /wd4251 /wd4819
  RC_FILE         = resources/icons/app.rc
}
macx {
  ICON = resources/icons/app.icns
  QMAKE_CXXFLAGS *= -mmacosx-version-min=10.6
  QMAKE_LFLAGS *= -mmacosx-version-min=10.6
  QMAKE_LFLAGS += -Wl,-rpath,@loader_path/../
  QMAKE_INFO_PLIST = resources/Info.plist
  translations.path = Contents/Resources
  QMAKE_BUNDLE_DATA += translations
  LIBS += -framework Cocoa
}
linux-* {
  QMAKE_RPATHDIR += \$\$ORIGIN
  QMAKE_RPATHDIR += \$\$ORIGIN/lib
  QMA_RPATH = $$join(QMAKE_RPATHDIR, ":")
  QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF -Wl,-z,origin \'-Wl,-rpath,$${QMA_RPATH}\'
  QMAKE_RPATHDIR =
  libraries.path = /lib
  libraries.files = $${LIBAV_PATH}/release_native/lib/libavcodec.so.* \
                    $${LIBAV_PATH}/release_native/lib/libavformat.so.* \
                    $${LIBAV_PATH}/release_native/lib/libavutil.so.* \
                    $${LIBAV_PATH}/release_native/lib/libswscale.so.* \
                    $${DEVIL_PATH}/release_native/lib/libIL.so.* \
                    $${DEVIL_PATH}/release_native/lib/libILU.so.* \
                    $${DEVIL_PATH}/release_native/lib/libILUT.so.* \
                    $$[QT_INSTALL_LIBS]/libQtCore.so.4 \
                    $$[QT_INSTALL_LIBS]/libQtGui.so.4 \
                    $$[QT_INSTALL_LIBS]/libQtOpenGL.so.4
  plugins.path = /plugins
  plugins.files = $$[QT_INSTALL_PLUGINS]/*
  INSTALLS += libraries plugins
}
!macx {
  translations.path = /locales
  INSTALLS += translations
}

SOURCES += main.cc \
    common/SceneWidget.cc \
    common/VPDFile.cc \
    common/SceneLoader.cc \
    common/LoggerWidget.cc \
    common/StringHelper.cc \
    common/Handles.cc \
    models/PMDMotionModel.cc \
    models/BoneMotionModel.cc \
    models/MorphMotionModel.cc \
    models/MotionBaseModel.cc \
    models/SceneMotionModel.cc \
    dialogs/BoneDialog.cc \
    dialogs/ExportVideoDialog.cc \
    dialogs/FrameWeightDialog.cc \
    dialogs/InterpolationDialog.cc \
    dialogs/PlaySettingDialog.cc \
    dialogs/FrameSelectionDialog.cc \
    dialogs/GravitySettingDialog.cc \
    dialogs/RenderOrderDialog.cc \
    dialogs/ShadowMapSettingDialog.cc \
    dialogs/BackgroundImageSettingDialog.cc \
    widgets/TimelineWidget.cc \
    widgets/MorphWidget.cc \
    widgets/CameraPerspectiveWidget.cc \
    widgets/TabWidget.cc \
    widgets/TimelineTabWidget.cc \
    widgets/TimelineTreeView.cc \
    widgets/LicenseWidget.cc \
    widgets/AssetWidget.cc \
    widgets/ModelTabWidget.cc \
    widgets/InterpolationGraphWidget.cc \
    widgets/ModelInfoWidget.cc \
    widgets/SceneLightWidget.cc \
    widgets/ModelSettingWidget.cc \
    MainWindow.cc \
    BoneUIDelegate.cc \
    ScenePlayer.cc \
    video/AVFactory.cc \
    common/Grid.cc \
    common/DebugDrawer.cc \
    common/BackgroundImage.cc

HEADERS  += \
    common/SceneWidget.h \
    common/Handles.h \
    common/SceneLoader.h \
    common/Grid.h \
    common/Application.h \
    common/VPDFile.h \
    common/InfoPanel.h \
    common/LoggerWidget.h \
    common/StringHelper.h \
    common/BackgroundImage.h \
    common/DebugDrawer.h \
    models/MotionBaseModel.h \
    models/BoneMotionModel.h \
    models/MorphMotionModel.h \
    models/SceneMotionModel.h \
    models/PMDMotionModel.h \
    dialogs/BoneDialog.h \
    dialogs/ExportVideoDialog.h \
    dialogs/FrameWeightDialog.h \
    dialogs/InterpolationDialog.h \
    dialogs/PlaySettingDialog.h \
    dialogs/FrameSelectionDialog.h \
    dialogs/GravitySettingDialog.h \
    dialogs/RenderOrderDialog.h \
    dialogs/ShadowMapSettingDialog.h \
    dialogs/BackgroundImageSettingDialog.h \
    widgets/MorphWidget.h \
    widgets/CameraPerspectiveWidget.h \
    widgets/TabWidget.h \
    widgets/TimelineTabWidget.h \
    widgets/TimelineTreeView.h \
    widgets/LicenseWidget.h \
    widgets/AssetWidget.h \
    widgets/ModelTabWidget.h \
    widgets/InterpolationGraphWidget.h \
    widgets/ModelInfoWidget.h \
    widgets/SceneLightWidget.h \
    widgets/ModelSettingWidget.h \
    widgets/TimelineWidget.h \
    video/IAudioDecoder.h \
    video/IVideoEncoder.h \
    MainWindow.h \
    BoneUIDelegate.h \
    ScenePlayer.h \
    video/AVFactory.h \
    video/IAudioPlayer.h

!win32 {
    SOURCES += video/AudioDecoder.cc \
        video/AVCommon.cc \
        video/AudioPlayer.cc \
        video/VideoEncoder.cc
    HEADERS += video/AudioDecoder.h \
        video/AVCommon.h \
        video/AudioPlayer.h \
        video/VideoEncoder.h
    DEFINES += VPVM_ENABLE_VIDEO
    LIBS += -L$${PORTAUDIO_PATH}/$${BUILD_DIRECTORY_WITH_NATIVE_SUFFIX}/lib \
          -L$${LIBAV_PATH}/$${BUILD_DIRECTORY_WITH_NATIVE_SUFFIX}/lib \
          -lportaudio \
          -lavcodec \
          -lavformat \
          -lavutil \
          -lswscale
}

CODECFORTR = UTF-8
RESOURCES += resources/VPVM.qrc
TRANSLATIONS += resources/translations/VPVM.ts

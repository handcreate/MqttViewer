cmake_minimum_required(VERSION 3.19)
project(MqttViewer LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(ARA_LIBRARY_PATH "E:/cpp-libs")
# 设置cmake搜索路径
set(ENV{CMAKE_PREFIX_PATH} ${ARA_LIBRARY_PATH})
# 设置module搜索路径CMAKE_MODULE_PATH
list(APPEND CMAKE_MODULE_PATH ${ARA_LIBRARY_PATH}/cmake_find_configs)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

find_package(PahoMqttCpp REQUIRED)

qt_standard_project_setup()

qt_add_executable(MqttViewer
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    json_highlighter.h
    data_processor.h
    data_processor.cpp
    json_highlighter.cpp
    hostdialog.h
    hostdialog.cpp
    hostdialog.ui
    subscribedialog.h
    subscribedialog.cpp
    subscribedialog.ui
    colorwidget.h
    colorwidget.cpp
    common_utils.h
    mqttconnection.h
    mqttconnection.cpp
    resource.qrc
    chatinput.h
    chatinput.cpp
    messagelist.h
    messagelist.cpp
    messagecenter.h
    messagecenter.cpp
    popuppanel.h
    popuppanel.cpp
    app.rc
)

target_link_libraries(MqttViewer
    PRIVATE
        Qt::Core
        Qt::Widgets
        PahoMqttCpp::paho-mqttpp3-static
)

install(TARGETS MqttViewer
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET MqttViewer
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

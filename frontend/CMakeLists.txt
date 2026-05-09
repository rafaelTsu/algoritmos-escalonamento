find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(simulador
    main.cpp
    mainwindow.cpp
    mainwindow.h
    process_table_model.cpp
    process_table_model.h
    timeline_view.cpp
    timeline_view.h
)

target_link_libraries(simulador PRIVATE
    Qt6::Widgets
    scheduler_backend
)

set_target_properties(simulador PROPERTIES
    WIN32_EXECUTABLE OFF
    MACOSX_BUNDLE OFF
)

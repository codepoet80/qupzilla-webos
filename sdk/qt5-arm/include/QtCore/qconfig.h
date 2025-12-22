/* Qt 5.9.7 configuration for ARM webOS cross-compilation */
#define QT_FEATURE_shared 1
#define QT_FEATURE_cross_compile 1
#define QT_FEATURE_framework -1
#define QT_FEATURE_rpath 1
#define QT_FEATURE_simulator_and_device -1
#define QT_FEATURE_thread 1

/* Required features for QT_CONFIG macro */
#define QT_FEATURE_topleveldomain 1
#define QT_FEATURE_idn 1

/* ARM NEON support */
#define QT_COMPILER_SUPPORTS_NEON 1

#define QT_FEATURE_appstore_compliant -1
#define QT_FEATURE_debug_and_release -1
#define QT_FEATURE_build_all -1
#define QT_FEATURE_c__11 1
#define QT_FEATURE_c__14 1
#define QT_FEATURE_c__17 -1
#define QT_FEATURE_c__1z -1
#define QT_FEATURE_c__2a -1
#define QT_FEATURE_c__2b -1
#define QT_FEATURE_c99 1
#define QT_FEATURE_c11 1
#define QT_FEATURE_future 1
#define QT_FEATURE_concurrent 1
#define QT_FEATURE_pkg_config 1
#define QT_FEATURE_force_asserts -1
#define QT_LARGEFILE_SUPPORT 64
#define QT_VISIBILITY_AVAILABLE true
#define QT_REDUCE_RELOCATIONS true
#define QT_FEATURE_separate_debug_info -1
#define QT_FEATURE_signaling_nan 1
#define QT_FEATURE_static -1

/* Qt 5.9.7 version - must match ARM libraries */
#define QT_VERSION_STR "5.9.7"
#define QT_VERSION_MAJOR 5
#define QT_VERSION_MINOR 9
#define QT_VERSION_PATCH 7

/* No SSE/AVX on ARM */
#undef QT_COMPILER_SUPPORTS_SSE2
#undef QT_COMPILER_SUPPORTS_SSE3
#undef QT_COMPILER_SUPPORTS_SSSE3
#undef QT_COMPILER_SUPPORTS_SSE4_1
#undef QT_COMPILER_SUPPORTS_SSE4_2
#undef QT_COMPILER_SUPPORTS_AVX
#undef QT_COMPILER_SUPPORTS_AVX2

/* Disabled features for webOS */
#define QT_NO_DBUS
#define QT_FEATURE_dbus -1

/* OpenGL ES 2 for webOS */
#define QT_OPENGL_ES
#define QT_OPENGL_ES_2
#define QT_FEATURE_opengles2 1
#define QT_FEATURE_opengl 1

/* Features enabled in the ARM build */
#define QT_FEATURE_accessibility 1
#define QT_FEATURE_animation 1
#define QT_FEATURE_cxx11_future 1
#define QT_FEATURE_datetimeparser 1
#define QT_FEATURE_easingcurve 1
#define QT_FEATURE_filesystemiterator 1
#define QT_FEATURE_filesystemwatcher 1
#define QT_FEATURE_gestures 1
#define QT_FEATURE_itemmodel 1
#define QT_FEATURE_itemmodeltester -1
#define QT_FEATURE_library 1
#define QT_FEATURE_mimetype 1
#define QT_FEATURE_processenvironment 1
#define QT_FEATURE_process 1
#define QT_FEATURE_properties 1
#define QT_FEATURE_proxymodel 1
#define QT_FEATURE_qml_debug 1
#define QT_FEATURE_regularexpression 1
#define QT_FEATURE_settings 1
#define QT_FEATURE_sharedmemory 1
#define QT_FEATURE_sortfilterproxymodel 1
#define QT_FEATURE_statemachine 1
#define QT_FEATURE_stringlistmodel 1
#define QT_FEATURE_systemsemaphore 1
#define QT_FEATURE_temporaryfile 1
#define QT_FEATURE_textcodec 1
#define QT_FEATURE_timezone 1
#define QT_FEATURE_translation 1
#define QT_FEATURE_xmlstream 1
#define QT_FEATURE_xmlstreamreader 1
#define QT_FEATURE_xmlstreamwriter 1

/* Network features */
#define QT_FEATURE_bearermanagement 1
#define QT_FEATURE_ftp 1
#define QT_FEATURE_http 1
#define QT_FEATURE_localserver 1
#define QT_FEATURE_networkinterface 1
#define QT_FEATURE_networkproxy 1
#define QT_FEATURE_ssl 1
#define QT_FEATURE_udpsocket 1

/* GUI features */
#define QT_FEATURE_action 1
#define QT_FEATURE_clipboard 1
#define QT_FEATURE_colornames 1
#define QT_FEATURE_cssparser 1
#define QT_FEATURE_cursor 1
#define QT_FEATURE_desktopservices 1
#define QT_FEATURE_draganddrop 1
#define QT_FEATURE_highdpiscaling 1
#define QT_FEATURE_image_heuristic_mask 1
#define QT_FEATURE_image_text 1
#define QT_FEATURE_imageformat_bmp 1
#define QT_FEATURE_imageformat_jpeg 1
#define QT_FEATURE_imageformat_png 1
#define QT_FEATURE_imageformat_ppm 1
#define QT_FEATURE_imageformat_xbm 1
#define QT_FEATURE_imageformat_xpm 1
#define QT_FEATURE_imageio_text_loading 1
#define QT_FEATURE_movie 1
#define QT_FEATURE_picture 1
#define QT_FEATURE_pdf 1
#define QT_FEATURE_sessionmanager -1
#define QT_FEATURE_shortcut 1
#define QT_FEATURE_standarditemmodel 1
#define QT_FEATURE_systemtrayicon -1
#define QT_FEATURE_tabletevent 1
#define QT_FEATURE_texthtmlparser 1
#define QT_FEATURE_textodfwriter 1
#define QT_FEATURE_validator 1
#define QT_FEATURE_wheelevent 1

/* Widget features */
#define QT_FEATURE_abstractbutton 1
#define QT_FEATURE_abstractslider 1
#define QT_FEATURE_buttongroup 1
#define QT_FEATURE_calendarwidget 1
#define QT_FEATURE_checkbox 1
#define QT_FEATURE_colordialog 1
#define QT_FEATURE_columnview 1
#define QT_FEATURE_combobox 1
#define QT_FEATURE_commandlinkbutton 1
#define QT_FEATURE_completer 1
#define QT_FEATURE_contextmenu 1
#define QT_FEATURE_datawidgetmapper 1
#define QT_FEATURE_datetimeedit 1
#define QT_FEATURE_dial 1
#define QT_FEATURE_dialog 1
#define QT_FEATURE_dialogbuttonbox 1
#define QT_FEATURE_dockwidget 1
#define QT_FEATURE_errormessage 1
#define QT_FEATURE_filedialog 1
#define QT_FEATURE_fontcombobox 1
#define QT_FEATURE_fontdialog 1
#define QT_FEATURE_formlayout 1
#define QT_FEATURE_fscompleter 1
#define QT_FEATURE_graphicseffect 1
#define QT_FEATURE_graphicsview 1
#define QT_FEATURE_groupbox 1
#define QT_FEATURE_inputdialog 1
#define QT_FEATURE_keysequenceedit 1
#define QT_FEATURE_label 1
#define QT_FEATURE_lcdnumber 1
#define QT_FEATURE_lineedit 1
#define QT_FEATURE_listview 1
#define QT_FEATURE_listwidget 1
#define QT_FEATURE_mainwindow 1
#define QT_FEATURE_mdiarea 1
#define QT_FEATURE_menu 1
#define QT_FEATURE_menubar 1
#define QT_FEATURE_messagebox 1
#define QT_FEATURE_progressbar 1
#define QT_FEATURE_progressdialog 1
#define QT_FEATURE_pushbutton 1
#define QT_FEATURE_radiobutton 1
#define QT_FEATURE_rubberband 1
#define QT_FEATURE_scrollarea 1
#define QT_FEATURE_scrollbar 1
#define QT_FEATURE_scroller 1
#define QT_FEATURE_sizegrip 1
#define QT_FEATURE_slider 1
#define QT_FEATURE_spinbox 1
#define QT_FEATURE_splashscreen 1
#define QT_FEATURE_splitter 1
#define QT_FEATURE_stackedwidget 1
#define QT_FEATURE_statusbar 1
#define QT_FEATURE_statustip 1
#define QT_FEATURE_style_fusion 1
#define QT_FEATURE_syntaxhighlighter 1
#define QT_FEATURE_tabbar 1
#define QT_FEATURE_tableview 1
#define QT_FEATURE_tablewidget 1
#define QT_FEATURE_tabwidget 1
#define QT_FEATURE_textbrowser 1
#define QT_FEATURE_textedit 1
#define QT_FEATURE_toolbar 1
#define QT_FEATURE_toolbox 1
#define QT_FEATURE_toolbutton 1
#define QT_FEATURE_tooltip 1
#define QT_FEATURE_treeview 1
#define QT_FEATURE_treewidget 1
#define QT_FEATURE_undocommand 1
#define QT_FEATURE_undogroup 1
#define QT_FEATURE_undostack 1
#define QT_FEATURE_undoview 1
#define QT_FEATURE_whatsthis 1
#define QT_FEATURE_widgettextcontrol 1
#define QT_FEATURE_wizard 1

/* SQL */
#define QT_FEATURE_sql 1
#define QT_FEATURE_sqlmodel 1

/* Print support */
#define QT_FEATURE_printer 1
#define QT_FEATURE_printdialog 1
#define QT_FEATURE_printpreviewdialog 1
#define QT_FEATURE_printpreviewwidget 1

/* Quick/QML */
#define QT_FEATURE_qml_network 1

/* WebEngine */
#define QT_FEATURE_webengine_webchannel 1

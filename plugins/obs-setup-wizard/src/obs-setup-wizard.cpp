   
#include "obs-setup-wizard.h"

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/config-file.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

#include <QApplication>
#include <QMessageBox>
#include <QFrame>
#include <QGridLayout>
#include <QScrollArea>
#include <QFont>
#include <QPalette>
#include <QStyleOption>
#include <QPainter>


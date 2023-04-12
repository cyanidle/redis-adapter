TEMPLATE = subdirs
SUBDIRS += src \
           app \
           plugins
           
app.depends = src
plugins.depends = src

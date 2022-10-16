#!/usr/bin/env python2
import os
import sys
from subprocess import check_call, CalledProcessError
from time import time

from gimpfu import *
from gimpshelf import shelf
import gtk

sys.path.append(os.path.abspath(os.path.dirname(sys.argv[0])))
gen_pattern = None


class Plugin:
    _layerData = []
    _selectedLayer = 0
    _layersCopies = []

    def _progress_callback(self, text, fraction):
        self._progressbar.set_text(text)
        self._progressbar.set_fraction(fraction)
        while gtk.events_pending():
            gtk.main_iteration()

    def _setLayer(self):
        self._selectedLayerLabel.set_text('Number of copies of layer "%s"' % self._layersStore[self._selectedLayer][1])
        self._copiesNumberEntry.set_text(str(self._layersCopies[self._selectedLayer]))

    def _layerSelectHandler(self, view):
        try:
            self._selectedLayer = view.get_selected_items()[0][0]
        except IndexError:
            return
        if not self._sameCopies.get_active():
            self._setLayer()

    def _setSameCopies(self):
        copies = int(self._copiesNumberEntry.get_text())
        for i in range(len(self._layersCopies)):
            self._layersCopies[i] = copies

    def _copiesNumberChanged(self, entry):
        if self._sameCopies.get_active():
            self._setSameCopies()
        else:
            self._layersCopies[self._selectedLayer] = int(entry.get_text())

    def _toggleSameCopies(self, checkButton):
        if checkButton.get_active():
            self._setSameCopies()
            self._selectedLayerLabel.set_text('Number of copies')
        else:
            self._setLayer()

    def _initParams(self):
        if not shelf.has_key('params'):
            return
        self._boundingRectThreshold.set_text(str(shelf['params'][0]))
        if len(self._img.layers) == len(shelf['params'][1]):
            self._layersCopies = shelf['params'][1]
        self._allowReflection.set_active(shelf['params'][2])
        self._minPadding.set_text(str(shelf['params'][3]))
        self._gridResolution.set_text(str(shelf['params'][4]))
        self._maxAngle.set_value(shelf['params'][5])
        self._simpLevel.set_value(shelf['params'][6])
        self._initialStep.set_value(shelf['params'][7])
        self._minDistance.set_text(str(shelf['params'][8]))
        self._forceCloser.set_active(shelf['params'][9])
        self._accuracy.set_text(str(shelf['params'][10]))

    def run(self, img, _):
        builder = gtk.Builder()
        builder.add_from_file(os.path.join(os.path.dirname(os.path.realpath(__file__)), 'ui.glade'))
        self._window = builder.get_object('mainwindow')
        self._window.connect('destroy', gtk.main_quit)
        
        try:
            global gen_pattern
            from plugin import layer_data, gen_pattern
        except ImportError:
            if os.name == 'nt':
                try:
                    check_call([sys.executable.replace('pythonw.exe', 'python.exe'), '-m', 'easy_install',
                                'https://github.com/platofff/genpattern-gimp/releases/download/v1.0/numpy-1.16.6-py2.7-mingw.egg',
                                'https://github.com/platofff/genpattern-gimp/releases/download/v1.0/Shapely-1.7.1-py2.7-mingw.egg'])
                    msg = 'Required packages were installed. Please run plugin again.'
                except CalledProcessError as e:
                    msg = '"Shapely" and "numpy" installation failed.\nReturn code: %d.\n' \
                          'You should try to install them manually.' % e.returncode
            else:
                msg = 'Please install "Shapely" and "numpy" packages into your Python 2 environment'
            md = gtk.MessageDialog(self._window, gtk.DIALOG_DESTROY_WITH_PARENT, gtk.MESSAGE_INFO, gtk.BUTTONS_CLOSE,
                                   msg)
            md.connect('destroy', gtk.main_quit)
            md.run()
            md.destroy()

        self._img = img
        self._progressbar = builder.get_object('progressbar')
        self._sameCopies = builder.get_object('same_copies')
        self._boundingRectThreshold = builder.get_object('bounding_rect_threshold')
        self._allowReflection = builder.get_object('allow_reflection')
        self._minPadding = builder.get_object('min_padding')
        self._selectedLayerLabel = builder.get_object('selected_layer')
        self._copiesNumberEntry = builder.get_object('copies_number')
        self._gridResolution = builder.get_object('grid_resolution')
        self._maxAngle = builder.get_object('max_angle')
        self._forceCloser = builder.get_object('force_closer')
        self._simpLevel = builder.get_object('simp_level')
        self._initialStep = builder.get_object('initial_step')
        self._minDistance = builder.get_object('min_distance')
        self._accuracy = builder.get_object('accuracy')

        self._copiesNumberEntry.connect('value-changed', self._copiesNumberChanged)
        self._sameCopies.connect('toggled', self._toggleSameCopies)

        layersView = builder.get_object('layers_view')
        layersView.set_pixbuf_column(0)
        layersView.set_text_column(1)
        for layer in img.layers:
            self._layersCopies.append(5)
            self._layerData.append(layer_data(layer))

        self._layersStore = builder.get_object('layers_store')
        for i, layer in enumerate(self._layerData):
            height, width, _ = layer.shape
            pixbuf = gtk.gdk.pixbuf_new_from_data(layer.flatten('C').tobytes(), gtk.gdk.COLORSPACE_RGB,
                                                  True, 8, width, height, width * 4)

            res_width = int(round(float(img.layers[i].width) / float(img.layers[i].height) * 64.0))
            res_pixbuf = gtk.gdk.Pixbuf(gtk.gdk.COLORSPACE_RGB, True, 8, res_width, 64)
            pixbuf.scale(res_pixbuf, 0, 0, res_width, 64, 0, 0, float(res_width) / width, 64.0 / height,
                         gtk.gdk.INTERP_NEAREST)
            self._layersStore.append([res_pixbuf, img.layers[i].name[:25]])
        layersView.connect('selection-changed', self._layerSelectHandler)
        builder.get_object('run_button').connect('clicked', self._runButtonHandler)
        self._window.show_all()
        self._initParams()
        self._setLayer()
        gtk.main()

    def _runButtonHandler(self, btn):
        btn.set_sensitive(False)
        shelf['params'] = (
                            int(self._boundingRectThreshold.get_text()),
                            self._layersCopies,
                            self._allowReflection.get_active(),
                            int(self._minPadding.get_text()),
                            int(self._gridResolution.get_text()),
                            int(self._maxAngle.get_value()),
                            float(self._simpLevel.get_value()),
                            int(self._initialStep.get_value()),
                            int(self._minDistance.get_text()),
                            self._forceCloser.get_active(),
                            int(self._accuracy.get_text()),
                        )
        start_time = time()
        gen_pattern(self._img, *shelf['params'], progress_callback=self._progress_callback)
        md = gtk.MessageDialog(self._window, gtk.DIALOG_DESTROY_WITH_PARENT, gtk.MESSAGE_INFO, gtk.BUTTONS_CLOSE,
        'Finished in %f seconds' % (time() - start_time))
        md.connect('destroy', gtk.main_quit)
        md.run()
        md.destroy()
        self._window.destroy()


plugin = Plugin()

register(
    "python_fu_genpattern",
    "Make a random pattern texture from layers with transparent background",
    "Make a random pattern texture from layers with transparent background",
    "platofff",
    "platofff",
    "2022",
    "<Image>/Filters/Artistic/Random pattern texture...",
    "RGBA",
    [],
    [],
    plugin.run)

main()

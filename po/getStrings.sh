xgettext -k_ ../src/*.cpp -o test.pot
xgettext -j -k_ ../git/*.cpp -o test.pot
xgettext -j -k_ ../shutdown/*.cpp -o test.pot
xgettext -j --language=glade ../templates/gtkbuilder.*.xml -o test.pot

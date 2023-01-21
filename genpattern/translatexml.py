from gettext import gettext as lg
import xml.etree.ElementTree as ET
from io import BytesIO

def translate_xml(filepath):
    tree = ET.parse(filepath)
    root = tree.getroot()
    for node in root.findall(".//*[@translatable='yes']"):
        node.text = lg(node.text)
    buf = BytesIO()
    tree.write(buf)
    buf.seek(0)
    return buf.read()
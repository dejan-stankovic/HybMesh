#!/usr/bin/env python
from collections import OrderedDict


def xmlindent(elem, level=0):
    """ http://effbot.org/zone/element-lib.htm#prettyprint.
        It basically walks your tree and adds spaces and newlines so the tree i
        printed in a nice way
    """
    tabsym = "    "
    i = "\n" + level * tabsym
    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = i + tabsym
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
        for elem in elem:
            xmlindent(elem, level + 1)
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
    else:
        if level and (not elem.tail or not elem.tail.strip()):
            elem.tail = i


def multifield_list(nd, dim):
    """ ([list], int) -> [[sublist1], [sublist2], ...]

    rebuild plain array into 2d array
    nd - plain array
    dim - dimension of sublist.
    if "auto" then dimension is set before the start of sublist:
        [1,1, 3,2,2,1, 5,3,3,2,1,1] -> [[1], [2,2,5], [3,3,2,1,1]]
    """
    if dim == 2:
        it = iter(nd)
        return [list(k) for k in zip(it, it)]
    elif dim == 3:
        it = iter(nd)
        return [list(k) for k in zip(it, it, it)]
    elif dim == "auto":
        lst, ret = nd, []
        while lst:
            l1 = lst.pop(0)
            ret.append(lst[:l1])
            lst = lst[l1:]
        return ret
    else:
        lst, ret = nd, []
        while lst:
            ret.append(lst[:dim])
            lst = lst[dim:]
        return ret


def unique_name(stem, nms):
    if (stem not in nms):
        return stem
    else:
        #find last non digit
        index = len(stem) - 1
        while stem[index].isdigit() and index >= 0:
            index -= 1
        if index < 0:
            raise Exception("Invalid name")
        if index < len(stem) - 1:
            i = int(stem[index + 1:])
            stem = stem[:index + 1]
        else:
            i = 0
        i += 1
        #find the unique name
        while stem + str(i) in nms:
            i += 1
        return stem + str(i)


class NamedList(OrderedDict):
    """ Presents modified collection.OrderedDict class.
        It changes the value of the string key (adds number to the end)
        if it was already presented in key list. F.e.
            a = NamedList([("A",1)])
            a["A"]=2; a["A"]=3
        Leads to {"A":1, "A1":2, "A2":3} dictionary
    """
    def __setitem__(self, key, value):
        ' Sets NamedList[key] = val behavior '
        key = unique_name(key, self.keys())
        OrderedDict.__setitem__(self, key, value)

    def change_value(self, key, value):
        """ Changes value for specified key.
        Default behaviour of OrderedDict.__setitem__
        """
        OrderedDict.__setitem__(self, key, value)

    def change_key(self, key_old, key_new):
        ' Changes the key string without breaking the order '
        if (key_new == key_old or key_old not in self):
            return
        key_new = unique_name(key_new, self.keys())
        restored = []
        while len(self) > 0:
            (k, v) = self.popitem()
            if (k == key_old):
                OrderedDict.__setitem__(self, key_new, v)
                break
            else:
                restored.append((k, v))
        for (k, v) in reversed(restored):
            OrderedDict.__setitem__(self, k, v)

    def get_by_index(self, ind):
        '-> (key, value). Get kv pair for certain index '
        return self.items()[ind]

    def get_by_value(self, val):
        ' -> (index, key) for certain value . '
        ind = self.values().index(val)
        return ind, self.keys()[ind]

    def get_by_key(self, key):
        ' -> (index, value) for certain key '
        ind = self.keys().index(key)
        return ind, self.values()[ind]

    #places (key, value) in a certain position
    def insert(self, ind, key, value):
        ' inserts key,value at the specified position '
        key = unique_name(key, self.keys())
        restored = []
        k = len(self) - ind
        #remove all entries after ind
        for i in range(k):
            restored.append(self.popitem())
        #add current item
        OrderedDict.__setitem__(self, key, value)
        #add all removed items
        for (k, v) in reversed(restored):
            OrderedDict.__setitem__(self, k, v)

if __name__ == '__main__':
    import copy

    a = NamedList([("active", 1), ("passive", 2)])
    a["once"] = 4
    for name, val in a.items():
        print name, val
    b = copy.deepcopy(a)
    print b
    print "eof"
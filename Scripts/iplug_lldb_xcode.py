import lldb
import re

# USAGE
#
# EITHER (automatic)
# Run the setup_iplug_lldb_xcode.sh script that sits alongside this one to add this to the lldb import paths
# OR (manually) Put this line in your ~/.lldbinit file:
#  command script import [path]
# Where [path] is the full path to this file. For example:
#  command script import /Users/Iplug2/Scripts/iPlug_lldb_xcode.py

def __lldb_init_module(debugger, dict):

    # types that are opaque without this script
    debugger.HandleCommand('type summary add WDL_String -F iplug_lldb_xcode.string_summary -w iplug')
    debugger.HandleCommand('type synthetic add WDL_TypedBuf<.*> -x -l iplug_lldb_xcode.WDL_TypedBufChildrenProvider -w iplug')
    debugger.HandleCommand('type synthetic add WDL_PtrList<.*> -x -l iplug_lldb_xcode.WDL_PtrListChildrenProvider -w iplug')

    # types that should summarise on one line
    debugger.HandleCommand('type summary add --inline-children IRECT -w iplug')
    debugger.HandleCommand('type summary add --inline-children IColor -w iplug')
    debugger.HandleCommand('type summary add --inline-children IBlend -w iplug')
    debugger.HandleCommand('type summary add --inline-children IMouseMod -w iplug')

    # turn the category on
    debugger.HandleCommand('type category enable iplug')

# summary function for WDL_String

def string_summary(valueObject, dictionary):
    debugger = lldb.debugger
    target = debugger.GetSelectedTarget()
    char_ptr = target.FindFirstType("char").GetPointerType()
    s = valueObject.GetChildMemberWithName('m_hb').GetChildMemberWithName('m_buf').Cast(char_ptr).GetSummary()
    return s

# Reports synthetic chlider for WDL_TypedBuf

class WDL_TypedBufChildrenProvider:
    def __init__(self, valobj, internal_dict):
        typename = valobj.GetTypeName()
        base_type = re.match('WDL_TypedBuf<(.*)>', typename, re.S).group(1)
        debugger = lldb.debugger
        target = debugger.GetSelectedTarget()
        self.data_type = target.FindFirstType(base_type)
        self.valobj = valobj

    def num_children(self):
        try:
            buf = self.valobj.GetChildMemberWithName('m_hb').GetChildMemberWithName('m_buf').GetValueAsUnsigned(0)
            size = self.valobj.GetChildMemberWithName('m_hb').GetChildMemberWithName('m_size').GetValueAsUnsigned(0)
            data_size = self.data_type.GetByteSize();
            alloc = self.valobj.GetChildMemberWithName('m_hb').GetChildMemberWithName('m_alloc').GetValueAsUnsigned(0)

            # Make sure nothing is NULL
            if buf == 0 or alloc == 0:
                return 0

            if (size % data_size) != 0:
                return 0
            else:
                num_children = size / data_size
            return num_children
        except:
            return 0

    def get_child_index(self,name):
        try:
            return int(name.lstrip('[').rstrip(']'))
        except:
            return -1

    def get_child_at_index(self,index):
        if index < 0:
            return None
        if index >= self.num_children():
            return None
        try:
            offset = index * self.data_type.GetByteSize()
            buf = self.valobj.GetChildMemberWithName('m_hb').GetChildMemberWithName('m_buf').GetValueAsUnsigned()
            return self.valobj.CreateValueFromAddress('[' + str(index) + ']', buf + offset, self.data_type)
        except:
            return None

    def update(self):
        pass
        #this call should be used to update the internal state of this Python object whenever the state of the variables in LLDB changes.[1]

    def has_children(self):
        return True

# Reports synthetic chlider for WDL_PtrList

class WDL_PtrListChildrenProvider:
    def __init__(self, valobj, internal_dict):
        typename = valobj.GetTypeName()
        try:
            base_type = re.match('WDL_PtrList<(.*)>', typename, re.S).group(1)
        except:
            base_type = re.match('WDL_PtrList<(.*)>', typename, re.S)
        debugger = lldb.debugger
        target = debugger.GetSelectedTarget()
        self.data_type = target.FindFirstType(base_type)
        self.valobj = valobj

    def num_children(self):
        try:
            buf = self.valobj.GetChildMemberWithName('m_hb').GetChildMemberWithName('m_buf').GetValueAsUnsigned(0)
            size = self.valobj.GetChildMemberWithName('m_hb').GetChildMemberWithName('m_size').GetValueAsUnsigned(0)
            data_size = self.data_type.GetPointerType().GetByteSize();
            alloc = self.valobj.GetChildMemberWithName('m_hb').GetChildMemberWithName('m_alloc').GetValueAsUnsigned(0)

            # Make sure nothing is NULL
            if buf == 0 or alloc == 0:
                return 0

            if (size % data_size) != 0:
                return 0
            else:
                num_children = size / data_size
            return num_children
        except:
            return 0

    def get_child_index(self,name):
        try:
            return int(name.lstrip('[').rstrip(']'))
        except:
            return -1

    def get_child_at_index(self,index):
        if index < 0:
            return None
        if index >= self.num_children():
            return None
        try:
            offset = index * self.data_type.GetPointerType().GetByteSize()
            buf = self.valobj.GetChildMemberWithName('m_hb').GetChildMemberWithName('m_buf').GetValueAsUnsigned()
            return self.valobj.CreateValueFromAddress('[' + str(index) + ']', buf + offset, self.data_type.GetPointerType())
        except:
            return None

    def update(self):
        pass
        #this call should be used to update the internal state of this Python object whenever the state of the variables in LLDB changes.[1]

    def has_children(self):
        return True

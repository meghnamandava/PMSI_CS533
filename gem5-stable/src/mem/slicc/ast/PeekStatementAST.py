# Copyright (c) 1999-2008 Mark D. Hill and David A. Wood
# Copyright (c) 2009 The Hewlett-Packard Development Company
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from slicc.ast.StatementAST import StatementAST
from slicc.symbols import Var

class PeekStatementAST(StatementAST):
    def __init__(self, slicc, queue_name, type_ast, pairs, statements, method):
        super(PeekStatementAST, self).__init__(slicc, pairs)

        self.queue_name = queue_name
        self.type_ast = type_ast
        self.statements = statements
        self.method = method

    def __repr__(self):
        return "[PeekStatementAST: %r queue_name: %r type: %r %r]" % \
               (self.method, self.queue_name, self.type_ast, self.statements)

    def generate(self, code, return_type):
        self.symtab.pushFrame()

        msg_type = self.type_ast.type

        # Add new local var to symbol table
        var = Var(self.symtab, "in_msg", self.location, msg_type, "(*in_msg_ptr)",
                  self.pairs)
        self.symtab.newSymbol(var)

        # Check the queue type
        self.queue_name.assertType("InPort")

        # Declare the new "in_msg_ptr" variable
        mtid = msg_type.ident

        qcode = self.queue_name.var.code
        code('''
{
    // Declare message
    const $mtid* in_msg_ptr M5_VAR_USED;
    in_msg_ptr = dynamic_cast<const $mtid *>(($qcode).${{self.method}}());
    assert(in_msg_ptr != NULL); // Check the cast result
''')

        # ANI: making changes to code
        # ANI: We should not allow the response msg to be blocked
        # Even on a locked RMW, the data for the RMW should be allowed to be sent from a remote core

        #if self.pairs.has_key("block_on") & (bool(str(mtid) != 'ResponseMsg') & bool(str(mtid) != 'RequestMsg')) :
        #if self.pairs.has_key("block_on") & (bool(str(mtid) != 'RequestMsg')) :
        if self.pairs.has_key("block_on"):
            address_field = self.pairs['block_on']
            if (str(mtid) == 'RubyRequest'):
                code('''
    if (m_is_blocking &&
        (m_block_map.count(in_msg_ptr->m_$address_field) == 1) &&
        (m_block_map[in_msg_ptr->m_$address_field] == &$qcode) &&       
        !(in_msg_ptr->m_PType == RubyRequestType::RubyRequestType_Locked_RMW_Write)) {
            $qcode.delayHead();
            DPRINTF(RubySlicc, "Ruby request Delaying in_msg_ptr %s \\n", *in_msg_ptr);
            continue;
    }
                ''')
            else:
                code('''
    if (m_is_blocking &&
        (m_block_map.count(in_msg_ptr->m_$address_field) == 1) &&
        (m_block_map[in_msg_ptr->m_$address_field] == &$qcode)) {
            $qcode.delayHead();
            DPRINTF(RubySlicc, "Delaying in_msg_ptr %s\\n", *in_msg_ptr);
            continue;
    }
                ''')

        if self.pairs.has_key("wake_up"):
            address_field = self.pairs['wake_up']
            code('''
    if (m_waiting_buffers.count(in_msg_ptr->m_$address_field) > 0) {
        wakeUpBuffers(in_msg_ptr->m_$address_field);
    }
            ''')

        # The other statements
        self.statements.generate(code, return_type)
        self.symtab.popFrame()
        code("}")

    def findResources(self, resources):
        self.statements.findResources(resources)

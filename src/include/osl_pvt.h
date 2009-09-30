/*
Copyright (c) 2009 Sony Pictures Imageworks, et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef OSL_PVT_H
#define OSL_PVT_H

#include "oslconfig.h"

#include "OpenImageIO/dassert.h"


#ifdef OSL_NAMESPACE
namespace OSL_NAMESPACE {
#endif

namespace OSL {
namespace pvt {

class ASTNode;
class ShadingExecution;


/// Kinds of shaders
///
enum ShaderType {
    ShadTypeUnknown, ShadTypeGeneric, ShadTypeSurface, 
    ShadTypeDisplacement, ShadTypeVolume, ShadTypeLight,
    ShadTypeLast
};


/// Convert a ShaderType to a human-readable name ("surface", etc.)
///
const char *shadertypename (ShaderType s);

/// Convert a ShaderType to a human-readable name ("surface", etc.)
///
ShaderType shadertype_from_name (const char *name);



/// Uses of shaders
///
enum ShaderUse {
    ShadUseSurface, ShadUseDisplacement, ShadUseVolume, ShadUseLight,
    ShadUseLast, ShadUseUnknown = ShadUseLast
};


/// Convert a ShaderUse to a human-readable name ("surface", etc.)
///
const char *shaderusename (ShaderUse s);

/// Convert a ShaderUse to a human-readable name ("surface", etc.)
///
ShaderUse shaderuse_from_name (const char *name);



/// Data type for flags that indicate on a point-by-point basis whether
/// we want computations to be performed.
typedef unsigned char Runflag;

/// Pre-defined values for Runflag's.
///
enum RunflagVal { RunflagOff = 0, RunflagOn = 255 };





/// Kinds of symbols
///
enum SymType {
    SymTypeParam, SymTypeOutputParam,
    SymTypeLocal, SymTypeTemp, SymTypeGlobal, SymTypeConst,
    SymTypeFunction, SymTypeType
};



/// Light-weight way to describe types for the compiler -- simple types,
/// closures, or the ID of a structure.
class TypeSpec {
public:
    /// Default ctr of TypeSpec (unknown type)
    ///
    TypeSpec ()
        : m_simple(TypeDesc::UNKNOWN), m_structure(0), m_closure(false)
    { }

    /// Construct a TypeSpec that represents an ordinary simple type
    /// (including arrays of simple types).
    TypeSpec (TypeDesc simple)
        : m_simple(simple), m_structure(0), m_closure(false)
    { }

    /// Construct a TypeSpec representing a closure (pass closure=true)
    /// of a simple type.
    TypeSpec (TypeDesc simple, bool closure)
        : m_simple(simple), m_structure(0), m_closure(closure)
    { }

    /// Construct a TypeSpec describing a struct or array of structs,
    /// by supplying the struct name, structure id, and array length
    /// (if it's an array of structures).
    TypeSpec (const char *name, int structid, int arraylen=0)
        : m_simple(TypeDesc::UNKNOWN, arraylen), m_structure((short)structid),
          m_closure(false)
    { }

    /// Express the type as a string
    ///
    std::string string () const;

    /// Assignment of a simple TypeDesc to a full TypeSpec.
    ///
    const TypeSpec & operator= (const TypeDesc simple) {
        m_simple = simple;
        m_structure = 0;
        m_closure = false;
        return *this;
    }

    /// Are two TypeSpec's identical?
    ///
    bool operator== (const TypeSpec &x) const {
        return (m_simple == x.m_simple && m_structure == x.m_structure &&
                m_closure == x.m_closure);
    }
    /// Are two TypeSpec's different?
    ///
    bool operator!= (const TypeSpec &x) const { return ! (*this == x); }

    /// Return just the simple type underlying this TypeSpec -- only works
    /// reliable if it's not a struct, a struct will return an UNKNOWN type.
    const TypeDesc &simpletype () const { return m_simple; }

    /// Is this typespec a closure?  (N.B. if so, you can find out what
    /// kind of closure it is with simpletype()).
    bool is_closure () const { return m_closure; }

    /// Is this typespec a single structure?  Caveat: Returns false if
    /// it's an array of structs.  N.B. You can find out which struct
    /// with structure().
    bool is_structure () const { return m_structure > 0 && !is_array(); }

    /// Return the structure ID of this typespec, or 0 if it's not a
    /// struct.
    int structure () const { return m_structure; }

    /// Is this an array (either a simple array, or an array of structs)?
    ///
    bool is_array () const { return m_simple.arraylen != 0; }

    /// Returns the length of the array, or 0 if not an array.
    ///
    int arraylength () const { return m_simple.arraylen; }

    /// Alter this typespec to make it into an array of the given length
    /// (including 0 -> make it not be an array).  The basic type (not
    /// counting its array length) is unchanged.
    void make_array (int len) { m_simple.arraylen = len; }

    /// For an array, return the TypeSpec of an individual element of the
    /// array.  For a non-array, just return the type.
    TypeSpec elementtype () const { TypeSpec t = *this; t.make_array (0); return t; }

    /// Return the aggregateness of the underlying simple type (SCALAR,
    /// VEC3, or MATRIX44).
    TypeDesc::AGGREGATE aggregate () const { return (TypeDesc::AGGREGATE)m_simple.aggregate; }

    // Note on the is_<simple_type> routines:
    // We don't need to explicitly check for !is_struct(), since the
    // m_simple is always UNKNOWN for structures.

    /// Is it a simple scalar int?
    ///
    bool is_int () const {
//        ASSERT (! is_closure() && "Don't call this if it could be a closure");
        return m_simple == TypeDesc::TypeInt && !is_closure();
    }

    /// Is it a simple scalar float?
    ///
    bool is_float () const {
        ASSERT (! is_closure() && "Don't call this if it could be a closure");
        return m_simple == TypeDesc::TypeFloat && !is_closure();
    }

    /// Is it a simple scalar float?
    ///
    bool is_color () const {
        ASSERT (! is_closure() && "Don't call this if it could be a closure");
        return m_simple == TypeDesc::TypeColor && !is_closure();
    }

    /// Is it a simple string?
    ///
    bool is_string () const {
        ASSERT (! is_closure() && "Don't call this if it could be a closure");
        return m_simple == TypeDesc::TypeString && !is_closure();
    }

    /// Is it a void?
    ///
    bool is_void () const {
        return m_simple == TypeDesc::NONE;
    }

    /// Is it a simple triple (color, point, vector, or normal)?
    ///
    bool is_triple () const {
        ASSERT (! is_closure() && "Don't call this if it could be a closure");
        return ! is_closure() && 
            (m_simple == TypeDesc::TypeColor ||
             m_simple == TypeDesc::TypePoint ||
             m_simple == TypeDesc::TypeVector ||
             m_simple == TypeDesc::TypeNormal);
    }

    /// Is this a simple type based on floats (including color/vector/etc)?  
    /// This will return false for a closure or array (even if of floats)
    /// or struct.
    bool is_floatbased () const {
        ASSERT (! is_closure() && "Don't call this if it could be a closure");
        return ! is_closure() && ! is_array() &&
            m_simple.basetype == TypeDesc::FLOAT;
    }

    /// Is it a simple numeric type (based on float or int, even if an
    /// aggregate)?  This is false for a closure or array (even if of
    /// an underlying numeric type) or struct.
    bool is_numeric () const {
        ASSERT (! is_closure() && "Don't call this if it could be a closure");
        return ! is_closure() && ! is_array() &&
            (m_simple.basetype == TypeDesc::FLOAT || m_simple.basetype == TypeDesc::INT);
    }

    bool is_scalarnum () const {
        ASSERT (! is_closure() && "Don't call this if it could be a closure");
        return is_numeric() && m_simple.aggregate == TypeDesc::SCALAR;
    }

    /// Is it a simple straight-up single int or float)?
    ///
    bool is_int_or_float () const { return is_scalarnum(); }

    /// Is it a simple vector-like triple (point, vector, or normal, but
    /// not an array or closure)?
    bool is_vectriple () const {
        ASSERT (! is_closure() && "Don't call this if it could be a closure");
        return ! is_closure() && 
            (m_simple == TypeDesc::TypePoint ||
             m_simple == TypeDesc::TypeVector ||
             m_simple == TypeDesc::TypeNormal);
    }

    /// Is it based on a vector-like triple (point, vector, or normal)?
    /// (It's ok for it to be an array or closure.)
    bool is_vectriple_based () const {
        return (m_simple.elementtype() == TypeDesc::TypePoint ||
                m_simple.elementtype() == TypeDesc::TypeVector ||
                m_simple.elementtype() == TypeDesc::TypeNormal);
    }

    /// Is it a simple matrix (but not an array or closure)?
    ///
    bool is_matrix () const {
        ASSERT (! is_closure() && "Don't call this if it could be a closure");
        return ! is_closure() && 
            m_simple == TypeDesc::TypeMatrix;
    }

    /// Is it a color closure?
    ///
    bool is_color_closure () const {
        return is_closure() && (m_simple == TypeDesc::TypeColor);
    }

    /// Types are equivalent if they are identical, or if both are
    /// vector-like (and match their array-ness and closure-ness).
    friend bool equivalent (const TypeSpec &a, const TypeSpec &b) {
        return (a == b) || 
            (a.is_vectriple_based() && b.is_vectriple_based() &&
             a.is_closure() == b.is_closure() &&
             a.arraylength() == b.arraylength());
    }

    /// Is type b is assignable to a?  It is if they are the equivalent(),
    /// or if a is a float or float-aggregate and b is a float or int.
    friend bool assignable (const TypeSpec &a, const TypeSpec &b) {
        return equivalent (a, b) || 
            (a.is_floatbased() && (b.is_float() || b.is_int()));
    }

private:
    TypeDesc m_simple;     ///< Data if it's a simple type
    short m_structure;     ///< 0 is not a structure, >=1 for structure id
    bool  m_closure;       ///< Is it a closure? (m_simple also used)
};



/// The compiler (or runtime) record of a single symbol (identifier) and
/// all relevant information about it.
class Symbol {
public:
    Symbol (ustring name, const TypeSpec &datatype, SymType symtype,
            ASTNode *declaration_node=NULL) 
        : m_data(NULL), m_step(0), m_size((int)datatype.simpletype().size()),
          m_name(name), m_typespec(datatype), m_symtype(symtype),
          m_has_derivs(false), m_const_initializer(false),
          m_scope(0), m_dataoffset(-1), 
          m_node(declaration_node), m_alias(NULL)
          
    { }
    virtual ~Symbol () { }

    /// The symbol's (unmangled) name, guaranteed unique only within the
    /// symbol's declaration scope.
    const ustring &name () const { return m_name; }

    /// The symbol's name, mangled to incorporate the scope so it will be
    /// a globally unique name.
    std::string mangled () const;

    /// Data type of this symbol.
    ///
    const TypeSpec &typespec () const { return m_typespec; }

    /// Kind of symbol this is (param, local, etc.)
    ///
    SymType symtype () const { return m_symtype; }

    /// Numerical ID of the scope in which this symbol was declared.
    ///
    int scope () const { return m_scope; }

    /// Set the scope of this symbol to s.
    ///
    void scope (int s) { m_scope = s; }

    /// Return teh AST node containing the declaration of this symbol.
    /// Use with care!
    ASTNode *node () const { return m_node; }

    /// Is this symbol a function?
    ///
    bool is_function () const { return m_symtype == SymTypeFunction; }

    /// Is this symbol a structure?
    ///
    bool is_structure () const { return m_symtype == SymTypeType; }

    /// Return a ptr to the symbol that this really refers to, tracing
    /// aliases back all the way until it finds a symbol that isn't an
    /// alias for anything else.
    Symbol *dealias () const {
        Symbol *s = const_cast<Symbol *>(this);
        while (s->m_alias)
            s = s->m_alias;
        return s;
    }

    /// Establish that this symbol is really an alias for another symbol.
    ///
    void alias (Symbol *other) {
        DASSERT (other != this);  // circular alias would be bad
        m_alias = other;
    }

    /// Return a string representation ("param", "global", etc.) of the
    /// SymType s.
    static const char *symtype_shortname (SymType s);

    /// Return a string representation ("param", "global", etc.) of this
    /// symbol.
    const char *symtype_shortname () const {
        return symtype_shortname(m_symtype);
    }

    /// Return a pointer to the symbol's data.
    ///
    void *data () const { return m_data; }

    /// Specify the location of the symbol's data.
    ///
    void data (void *d) { m_data = d; }

    void dataoffset (int d) { m_dataoffset = d; }
    int dataoffset () const { return m_dataoffset; }

    int step () const { return m_step; }
    void step (int newstep) { m_step = newstep; }

    bool is_uniform () const { return m_step == 0; }
    bool is_varying () const { return m_step != 0; }

    bool has_derivs () const { return m_has_derivs; }
    int deriv_step () const { return m_size; /*m_deriv_step;*/ }
    void has_derivs (bool new_derivs) {
        m_has_derivs = new_derivs;
    }
    int size () const { return m_size; }
    void size (size_t newsize) { m_size = (int)newsize; }

    /// Return the size for each point, including derivs.
    ///
    int derivsize () const { return m_has_derivs ? 3*m_size : m_size; }

protected:
    void *m_data;               ///< Pointer to the data
    int m_step;                 ///< Step (in bytes) from point to point
    int m_size;                 ///< Size of data (in bytes)
    ustring m_name;             ///< Symbol name (unmangled)
    TypeSpec m_typespec;        ///< Data type of the symbol
    SymType m_symtype;          ///< Kind of symbol (param, local, etc.)
    bool m_has_derivs;          ///< Step to derivs (0 == has no derivs)
    bool m_const_initializer;   ///< initializer is a constant expression
    int m_scope;                ///< Scope where this symbol was declared
    int m_dataoffset;           ///< Offset of the data (-1 for unknown)
    ASTNode *m_node;            ///< Ptr to the declaration of this symbol
    Symbol *m_alias;            ///< Another symbol that this is an alias for
};



typedef std::vector<Symbol> SymbolVec;
typedef std::vector<Symbol *> SymbolPtrVec;


/// Function pointer to a shadeop implementation
///
typedef void (*OpImpl) (ShadingExecution *exec, int nargs, const int *args,
                        Runflag *runflags, int beginpoint, int endpoint);



/// Intermediate Represenatation opcode
///
class Opcode {
public:
    Opcode (ustring op, ustring method, size_t firstarg=0, size_t nargs=0)
        : m_op(op), m_firstarg((int)firstarg), m_nargs((int)nargs),
          m_method(method), m_impl(NULL)
    {
        m_jump[0] = -1;
        m_jump[1] = -1;
        m_jump[2] = -1;
        m_jump[3] = -1;
    }
    ustring opname () const { return m_op; }
    int firstarg () const { return m_firstarg; }
    int nargs () const { return m_nargs; }
    ustring method () const { return m_method; }
    void source (ustring sourcefile, int sourceline) {
        m_sourcefile = sourcefile;
        m_sourceline = sourceline;
    }
    ustring sourcefile () const { return m_sourcefile; }
    int sourceline () const { return m_sourceline; }

    void set_args (size_t firstarg, size_t nargs) {
        m_firstarg = (int) firstarg;
        m_nargs = (int) nargs;
    }

    /// Set the jump addresses (-1 means no jump)
    ///
    void set_jump (int jump0=-1, int jump1=-1, int jump2=-1, int jump3=-1) {
        m_jump[0] = jump0;
        m_jump[1] = jump1;
        m_jump[2] = jump2;
        m_jump[3] = jump3;
    }

    void add_jump (int target) {
        for (unsigned int i = 0;  i < max_jumps;  ++i)
            if (m_jump[i] < 0) {
                m_jump[i] = target;
                return;
            }
    }

    /// Return the i'th jump target address (-1 for none).
    ///
    int jump (int i) const { return m_jump[i]; }

    /// Maximum jump targets an op can have.
    ///
    static const unsigned int max_jumps = 4;

    void implementation (OpImpl impl) { m_impl = impl; }
    OpImpl implementation () const { return m_impl; }

    /// Execute the op!
    ///
    void operator() (ShadingExecution *exec, int nargs, const int *args,
                     Runflag *runflags, int beginpoint, int endpoint) {
        m_impl (exec, m_nargs, args, runflags, beginpoint, endpoint);
    }

private:
    ustring m_op;                   ///< Name of opcode
    int m_firstarg;                 ///< Index of first argument
    int m_nargs;                    ///< Total number of arguments
    ustring m_method;               ///< Which param or method this code is for
    int m_jump[max_jumps];          ///< Jump addresses (-1 means none)
    ustring m_sourcefile;           ///< Source filename for this op
    int m_sourceline;               ///< Line of source code for this op
    OpImpl m_impl;                  ///< Implementation of this op
};


typedef std::vector<Opcode> OpcodeVec;



}; // namespace OSL::pvt
}; // namespace OSL

#ifdef OSL_NAMESPACE
}; // end namespace OSL_NAMESPACE
using namespace OSL_NAMESPACE;
#endif

#endif /* OSL_PVT_H */

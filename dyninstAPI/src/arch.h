/*
 * Copyright (c) 1996-2008 Barton P. Miller
 * 
 * We provide the Paradyn Parallel Performance Tools (below
 * described as "Paradyn") on an AS IS basis, and do not warrant its
 * validity or performance.  We reserve the right to update, modify,
 * or discontinue this software at any time.  We shall have no
 * obligation to supply such updates or modifications or any other
 * form of support to you.
 * 
 * This license is for research uses.  For such uses, there is no
 * charge. We define "research use" to mean you may freely use it
 * inside your organization for whatever purposes you see fit. But you
 * may not re-distribute Paradyn or parts of Paradyn, in any form
 * source or binary (including derivatives), electronic or otherwise,
 * to any other organization or entity without our permission.
 * 
 * (for other uses, please contact us at paradyn@cs.wisc.edu)
 * 
 * All warranties, including without limitation, any warranty of
 * merchantability or fitness for a particular purpose, are hereby
 * excluded.
 * 
 * By your use of Paradyn, you understand and agree that we (or any
 * other person or entity with proprietary rights in Paradyn) are
 * under no obligation to provide either maintenance services,
 * update services, notices of latent defects, or correction of
 * defects for Paradyn.
 * 
 * Even if advised of the possibility of such damages, under no
 * circumstances shall we (or any other person or entity with
 * proprietary rights in the software licensed hereunder) be liable
 * to you or any third party for direct, indirect, or consequential
 * damages of any character regardless of type of action, including,
 * without limitation, loss of profits, loss of use, loss of good
 * will, or computer failure or malfunction.  You agree to indemnify
 * us (and any other person or entity with proprietary rights in the
 * software licensed hereunder) for any and all liability it may
 * incur to third parties resulting from your use of Paradyn.
 */

// Architecture include. Use this one instead of arch-<platform>

#if !defined(arch_h)
#define arch_h

#include <assert.h>
#include <vector>

#include "dyninstAPI/src/patch.h"

#if defined(sparc_sun_sunos4_1_3) \
 || defined(sparc_sun_solaris2_4)
#include "arch-sparc.h"

#elif defined(arch_power)
#include "arch-power.h"

#elif defined(i386_unknown_solaris2_5) \
   || defined(i386_unknown_nt4_0) \
   || defined(i386_unknown_linux2_0) \
   || defined(x86_64_unknown_linux2_4)
#include "arch-x86.h"

#elif defined(ia64_unknown_linux2_4)
#include "arch-ia64.h"

#else
#error "unknown architecture"

#endif

#include "dyninstAPI/src/bitArray.h"

class dyn_lwp;
class dyn_thread;
class process;
class AddressSpace;
class instPoint;
class registerSpace;
class regTracker_t;
class AstNode;
class Emitter;
class pcRelRegion;
class int_function;
class generatedCodeObject;
class baseTrampInstance;

// Code generation
// This class wraps the actual code generation mechanism: we keep a buffer
// and a pointer to where we are in the buffer. This may vary by platform,
// hence the typedef wrappers.
class codeGen {
    // Instruction modifies these.
    friend class instruction;
    friend class AstNode;
 public:
    // Default constructor -- makes an empty generation area
    codeGen();
    // Make a generation buffer with the given size
    codeGen(unsigned size);
    // Use a preallocated buffer
    codeGen(codeBuf_t *buf, int size);
    ~codeGen();

    // Copy constructor. Deep-copy -- allocates
    // a new buffer
    codeGen(const codeGen &);

    // We consider our pointer to either be the start
    // of the buffer, or NULL if the buffer is empty
    bool operator==(void *ptr) const;
    bool operator!=(void *ptr) const;

    // Assignment....
    codeGen &operator=(const codeGen &param);

    // Initialize the current using the argument as a "template"
    void applyTemplate(codeGen &codeTemplate);
    static codeGen baseTemplate;

    // Allocate a certain amount of space
    void allocate(unsigned);
    // And invalidate
    void invalidate();

    // Finally, tighten down the memory usage. This frees the 
    // buffer if it's bigger than necessary and copies everything
    // to a new fixed buffer.
    void finalize();
    
    // Copy a buffer into here and move the offset
    void copy(const void *buf, const unsigned size);
    // Similar, but slurp from the start of the parameter
    void copy(codeGen &gen);

    // How much space are we using?
    unsigned used() const;

    // Blind pointer to the start of the code area
    void *start_ptr() const;
    // With ptr() and used() you can copy into the mutatee.

    // Pointer to the current location...
    void *cur_ptr() const;

    // And pointer to a given offset
    void *get_ptr(unsigned offset) const;

    // For things that make a copy of the current pointer and
    // play around with it. This recalculates the current offset
    // based on a new pointer
    void update(codeBuf_t *ptr);

    // Set the offset at a particular location.
    void setIndex(codeBufIndex_t offset);

    codeBufIndex_t getIndex() const;

    // Move up or down a certain amount
    void moveIndex(int disp);

    // To calculate a jump between the "from" and where we are
    static int getDisplacement(codeBufIndex_t from, codeBufIndex_t to);

    // For code generation -- given the current state of 
    // generation and a base address in the mutatee, 
    // produce a "current" address.
    Address currAddr() const;
    Address currAddr(Address base) const;
    
    enum { cgNOP, cgTrap, cgIllegal };

    void fill(unsigned fillSize, int fillType);
    // Since we have a known size
    void fillRemaining(int fillType);

    void setData(process *proc, dyn_thread *thr, dyn_lwp *lwp,
                 Address startAddr,
                 instPoint *point,
                 registerSpace *rs);

    //Add a new PCRelative region that should be generated after 
    // addresses are fixed
    void addPCRelRegion(pcRelRegion *reg);

    //Have each region generate code with this codeGen object being
    // placed at addr
    void applyPCRels(Address addr);

    //Return true if there are any active regions.
    bool hasPCRels() const;

    //Add a new patch point
    void addPatch(const relocPatch &p);

    //Create a patch into the codeRange
    void addPatch(void *dest, patchTarget *source, 
                  unsigned size = sizeof(Address),
                  relocPatch::patch_type_t ptype = relocPatch::abs,
                  Dyninst::Offset off = 0);

    std::vector<relocPatch> &allPatches();

    //Apply all patches that have been added
    void applyPatches();

    //void setProcess(process *p);
    void setAddrSpace(AddressSpace *a);
    void setThread(dyn_thread *t) { thr_ = t; }
    void setLWP(dyn_lwp *l) { lwp_ = l; }
    void setRegisterSpace(registerSpace *r) { rs_ = r; }
    void setAddr(Address a) { addr_ = a; }
    void setPoint(instPoint *i) { ip_ = i; }
    void setRegTracker(regTracker_t *t) { t_ = t; }
    void setCodeEmitter(Emitter *emitter) { emitter_ = emitter; }
    void setFunction(int_function *f) { f_ = f; }
    void setObj(generatedCodeObject *object) { obj_ = object; }
    void setBTI(baseTrampInstance *i) { bti_ = i; }

    dyn_lwp *lwp();
    dyn_thread *thread();
    //process *proc();
    AddressSpace *addrSpace();
    Address startAddr() const { return addr_; }
    instPoint *point();
    baseTrampInstance *bti() const { return bti_; }
    int_function *func();
    registerSpace *rs();
    regTracker_t *tracker();
    Emitter *codeEmitter();
    Emitter *emitter() { return codeEmitter(); } // A little shorter

    generatedCodeObject *obj();

    void beginTrackRegDefs();
    void endTrackRegDefs();
    const bitArray &getRegsDefined();
    void markRegDefined(Register r);
    bool isRegDefined(Register r);

    void setPCRelUseCount(int c) { pc_rel_use_count = c; }
    int getPCRelUseCount() const { return pc_rel_use_count; }
 private:
    codeBuf_t *buffer_;
    codeBufIndex_t offset_;
    unsigned size_;
    int pc_rel_use_count;

    Emitter *emitter_;
    bool allocated_;

    //process *proc_;
    AddressSpace *aSpace_;
    dyn_thread *thr_;
    dyn_lwp * lwp_;
    registerSpace *rs_;
    regTracker_t *t_;
    Address addr_;
    instPoint *ip_;
    int_function *f_;
    baseTrampInstance *bti_;
    bool isPadded_;

    bitArray regsDefined_;
    bool trackRegDefs_;

    generatedCodeObject *obj_;

    std::vector<relocPatch> patches_;
    std::vector<pcRelRegion *> pcrels_;
};

class pcRelRegion {
   friend class codeGen;
 protected:
   codeGen *gen;
   instruction orig_instruc;
   unsigned cur_offset;
   unsigned cur_size;
 public:
   pcRelRegion(const instruction &i);
   virtual unsigned apply(Address addr) = 0;
   virtual unsigned maxSize() = 0;
   virtual bool canPreApply();
   virtual ~pcRelRegion();
}; 

// For platforms that require bit-twiddling. These should go away in the future.
#define GET_PTR(insn, gen) codeBuf_t *insn = (codeBuf_t *)(gen).cur_ptr()
#define SET_PTR(insn, gen) (gen).update(insn)
#define REGET_PTR(insn, gen) insn = (codeBuf_t *)(gen).cur_ptr()

#endif

//===--- Scalarizer.cpp - Scalarize vector operations ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass converts vector operations into scalar operations, in order
// to expose optimization opportunities on the individual scalar operations.
// It is mainly intended for targets that do not have vector units, but it
// may also be useful for revectorizing code to different vector widths.
//
//===----------------------------------------------------------------------===//
#include "klee/Config/Version.h"

// This is taken from r195471 in LLVM. This unfortunately was introduced just
// after LLVM branched for 3.4 so it has been copied into KLEE's source tree.
// We only use this for LLVM 3.4 because newer LLVM's have this pass in-tree.
#if LLVM_VERSION_CODE == LLVM_VERSION(3, 4)

#define DEBUG_TYPE "scalarizer"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/InstVisitor.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

namespace {
// Used to store the scattered form of a vector.
typedef SmallVector<Value *, 8> ValueVector;

// Used to map a vector Value to its scattered form.  We use std::map
// because we want iterators to persist across insertion and because the
// values are relatively large.
typedef std::map<Value *, ValueVector> ScatterMap;

// Lists Instructions that have been replaced with scalar implementations,
// along with a pointer to their scattered forms.
typedef SmallVector<std::pair<Instruction *, ValueVector *>, 16> GatherList;

// Provides a very limited vector-like interface for lazily accessing one
// component of a scattered vector or vector pointer.
class Scatterer {
public:
  // Scatter V into Size components.  If new instructions are needed,
  // insert them before BBI in BB.  If Cache is nonnull, use it to cache
  // the results.
  Scatterer(BasicBlock *bb, BasicBlock::iterator bbi, Value *v,
            ValueVector *cachePtr = 0);

  // Return component I, creating a new Value for it if necessary.
  Value *operator[](unsigned I);

  // Return the number of components.
  unsigned size() const { return Size; }

private:
  BasicBlock *BB;
  BasicBlock::iterator BBI;
  Value *V;
  ValueVector *CachePtr;
  PointerType *PtrTy;
  ValueVector Tmp;
  unsigned Size;
};

// FCmpSpliiter(FCI)(Builder, X, Y, Name) uses Builder to create an FCmp
// called Name that compares X and Y in the same way as FCI.
struct FCmpSplitter {
  FCmpSplitter(FCmpInst &fci) : FCI(fci) {}
  Value *operator()(IRBuilder<> &Builder, Value *Op0, Value *Op1,
                    const Twine &Name) const {
    return Builder.CreateFCmp(FCI.getPredicate(), Op0, Op1, Name);
  }
  FCmpInst &FCI;
};

// ICmpSpliiter(ICI)(Builder, X, Y, Name) uses Builder to create an ICmp
// called Name that compares X and Y in the same way as ICI.
struct ICmpSplitter {
  ICmpSplitter(ICmpInst &ici) : ICI(ici) {}
  Value *operator()(IRBuilder<> &Builder, Value *Op0, Value *Op1,
                    const Twine &Name) const {
    return Builder.CreateICmp(ICI.getPredicate(), Op0, Op1, Name);
  }
  ICmpInst &ICI;
};

// BinarySpliiter(BO)(Builder, X, Y, Name) uses Builder to create
// a binary operator like BO called Name with operands X and Y.
struct BinarySplitter {
  BinarySplitter(BinaryOperator &bo) : BO(bo) {}
  Value *operator()(IRBuilder<> &Builder, Value *Op0, Value *Op1,
                    const Twine &Name) const {
    return Builder.CreateBinOp(BO.getOpcode(), Op0, Op1, Name);
  }
  BinaryOperator &BO;
};

// GEPSpliiter()(Builder, X, Y, Name) uses Builder to create
// a single GEP called Name with operands X and Y.
struct GEPSplitter {
  GEPSplitter() {}
  Value *operator()(IRBuilder<> &Builder, Value *Op0, Value *Op1,
                    const Twine &Name) const {
    return Builder.CreateGEP(Op0, Op1, Name);
  }
};

// Information about a load or store that we're scalarizing.
struct VectorLayout {
  VectorLayout() : VecTy(0), ElemTy(0), VecAlign(0), ElemSize(0) {}

  // Return the alignment of element I.
  uint64_t getElemAlign(unsigned I) { return MinAlign(VecAlign, I * ElemSize); }

  // The type of the vector.
  VectorType *VecTy;

  // The type of each element.
  Type *ElemTy;

  // The alignment of the vector.
  uint64_t VecAlign;

  // The size of each element.
  uint64_t ElemSize;
};

class Scalarizer : public FunctionPass, public InstVisitor<Scalarizer, bool> {
public:
  static char ID;

  Scalarizer() : FunctionPass(ID) {
    // HACK:
    // initializeScalarizerPass(*PassRegistry::getPassRegistry());
  }

  virtual bool doInitialization(Module &M);
  virtual bool runOnFunction(Function &F);

  // InstVisitor methods.  They return true if the instruction was scalarized,
  // false if nothing changed.
  bool visitInstruction(Instruction &) { return false; }
  bool visitSelectInst(SelectInst &SI);
  bool visitICmpInst(ICmpInst &);
  bool visitFCmpInst(FCmpInst &);
  bool visitBinaryOperator(BinaryOperator &);
  bool visitGetElementPtrInst(GetElementPtrInst &);
  bool visitCastInst(CastInst &);
  bool visitBitCastInst(BitCastInst &);
  bool visitShuffleVectorInst(ShuffleVectorInst &);
  bool visitPHINode(PHINode &);
  bool visitLoadInst(LoadInst &);
  bool visitStoreInst(StoreInst &);

private:
  Scatterer scatter(Instruction *, Value *);
  void gather(Instruction *, const ValueVector &);
  bool canTransferMetadata(unsigned Kind);
  void transferMetadata(Instruction *, const ValueVector &);
  bool getVectorLayout(Type *, unsigned, VectorLayout &);
  bool finish();

  template <typename T> bool splitBinary(Instruction &, const T &);

  ScatterMap Scattered;
  GatherList Gathered;
  unsigned ParallelLoopAccessMDKind;
  const DataLayout *TDL;
};

char Scalarizer::ID = 0;
} // end anonymous namespace

bool ScalarizeLoadStore = true; // HACK
/*
// This is disabled by default because having separate loads and stores makes
// it more likely that the -combiner-alias-analysis limits will be reached.
static cl::opt<bool> ScalarizeLoadStore
  ("scalarize-load-store", cl::Hidden, cl::init(false),
   cl::desc("Allow the scalarizer pass to scalarize loads and store"));

INITIALIZE_PASS(Scalarizer, "scalarizer", "Scalarize vector operations",
                false, false)
*/

Scatterer::Scatterer(BasicBlock *bb, BasicBlock::iterator bbi, Value *v,
                     ValueVector *cachePtr)
    : BB(bb), BBI(bbi), V(v), CachePtr(cachePtr) {
  Type *Ty = V->getType();
  PtrTy = dyn_cast<PointerType>(Ty);
  if (PtrTy)
    Ty = PtrTy->getElementType();
  Size = Ty->getVectorNumElements();
  if (!CachePtr)
    Tmp.resize(Size, 0);
  else if (CachePtr->empty())
    CachePtr->resize(Size, 0);
  else
    assert(Size == CachePtr->size() && "Inconsistent vector sizes");
}

// Return component I, creating a new Value for it if necessary.
Value *Scatterer::operator[](unsigned I) {
  ValueVector &CV = (CachePtr ? *CachePtr : Tmp);
  // Try to reuse a previous value.
  if (CV[I])
    return CV[I];
  IRBuilder<> Builder(BB, BBI);
  if (PtrTy) {
    if (!CV[0]) {
      Type *Ty =
          PointerType::get(PtrTy->getElementType()->getVectorElementType(),
                           PtrTy->getAddressSpace());
      CV[0] = Builder.CreateBitCast(V, Ty, V->getName() + ".i0");
    }
    if (I != 0)
      CV[I] =
          Builder.CreateConstGEP1_32(CV[0], I, V->getName() + ".i" + Twine(I));
  } else {
    // Search through a chain of InsertElementInsts looking for element I.
    // Record other elements in the cache.  The new V is still suitable
    // for all uncached indices.
    for (;;) {
      InsertElementInst *Insert = dyn_cast<InsertElementInst>(V);
      if (!Insert)
        break;
      ConstantInt *Idx = dyn_cast<ConstantInt>(Insert->getOperand(2));
      if (!Idx)
        break;
      unsigned J = Idx->getZExtValue();
      CV[J] = Insert->getOperand(1);
      V = Insert->getOperand(0);
      if (I == J)
        return CV[J];
    }
    CV[I] = Builder.CreateExtractElement(V, Builder.getInt32(I),
                                         V->getName() + ".i" + Twine(I));
  }
  return CV[I];
}

bool Scalarizer::doInitialization(Module &M) {
  ParallelLoopAccessMDKind =
      M.getContext().getMDKindID("llvm.mem.parallel_loop_access");
  return false;
}

bool Scalarizer::runOnFunction(Function &F) {
  TDL = getAnalysisIfAvailable<DataLayout>();
  for (Function::iterator BBI = F.begin(), BBE = F.end(); BBI != BBE; ++BBI) {
    BasicBlock *BB = BBI;
    for (BasicBlock::iterator II = BB->begin(), IE = BB->end(); II != IE;) {
      Instruction *I = II;
      bool Done = visit(I);
      ++II;
      if (Done && I->getType()->isVoidTy())
        I->eraseFromParent();
    }
  }
  return finish();
}

// Return a scattered form of V that can be accessed by Point.  V must be a
// vector or a pointer to a vector.
Scatterer Scalarizer::scatter(Instruction *Point, Value *V) {
  if (Argument *VArg = dyn_cast<Argument>(V)) {
    // Put the scattered form of arguments in the entry block,
    // so that it can be used everywhere.
    Function *F = VArg->getParent();
    BasicBlock *BB = &F->getEntryBlock();
    return Scatterer(BB, BB->begin(), V, &Scattered[V]);
  }
  if (Instruction *VOp = dyn_cast<Instruction>(V)) {
    // Put the scattered form of an instruction directly after the
    // instruction.
    BasicBlock *BB = VOp->getParent();
    return Scatterer(BB, llvm::next(BasicBlock::iterator(VOp)), V,
                     &Scattered[V]);
  }
  // In the fallback case, just put the scattered before Point and
  // keep the result local to Point.
  return Scatterer(Point->getParent(), Point, V);
}

// Replace Op with the gathered form of the components in CV.  Defer the
// deletion of Op and creation of the gathered form to the end of the pass,
// so that we can avoid creating the gathered form if all uses of Op are
// replaced with uses of CV.
void Scalarizer::gather(Instruction *Op, const ValueVector &CV) {
  // Since we're not deleting Op yet, stub out its operands, so that it
  // doesn't make anything live unnecessarily.
  for (unsigned I = 0, E = Op->getNumOperands(); I != E; ++I)
    Op->setOperand(I, UndefValue::get(Op->getOperand(I)->getType()));

  transferMetadata(Op, CV);

  // If we already have a scattered form of Op (created from ExtractElements
  // of Op itself), replace them with the new form.
  ValueVector &SV = Scattered[Op];
  if (!SV.empty()) {
    for (unsigned I = 0, E = SV.size(); I != E; ++I) {
      Instruction *Old = cast<Instruction>(SV[I]);
      CV[I]->takeName(Old);
      Old->replaceAllUsesWith(CV[I]);
      Old->eraseFromParent();
    }
  }
  SV = CV;
  Gathered.push_back(GatherList::value_type(Op, &SV));
}

// Return true if it is safe to transfer the given metadata tag from
// vector to scalar instructions.
bool Scalarizer::canTransferMetadata(unsigned Tag) {
  return (Tag == LLVMContext::MD_tbaa || Tag == LLVMContext::MD_fpmath ||
          Tag == LLVMContext::MD_tbaa_struct ||
          Tag == LLVMContext::MD_invariant_load ||
          Tag == ParallelLoopAccessMDKind);
}

// Transfer metadata from Op to the instructions in CV if it is known
// to be safe to do so.
void Scalarizer::transferMetadata(Instruction *Op, const ValueVector &CV) {
  SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
  Op->getAllMetadataOtherThanDebugLoc(MDs);
  for (unsigned I = 0, E = CV.size(); I != E; ++I) {
    if (Instruction *New = dyn_cast<Instruction>(CV[I])) {
      for (SmallVectorImpl<std::pair<unsigned, MDNode *>>::iterator
               MI = MDs.begin(),
               ME = MDs.end();
           MI != ME; ++MI)
        if (canTransferMetadata(MI->first))
          New->setMetadata(MI->first, MI->second);
      New->setDebugLoc(Op->getDebugLoc());
    }
  }
}

// Try to fill in Layout from Ty, returning true on success.  Alignment is
// the alignment of the vector, or 0 if the ABI default should be used.
bool Scalarizer::getVectorLayout(Type *Ty, unsigned Alignment,
                                 VectorLayout &Layout) {
  if (!TDL)
    return false;

  // Make sure we're dealing with a vector.
  Layout.VecTy = dyn_cast<VectorType>(Ty);
  if (!Layout.VecTy)
    return false;

  // Check that we're dealing with full-byte elements.
  Layout.ElemTy = Layout.VecTy->getElementType();
  if (TDL->getTypeSizeInBits(Layout.ElemTy) !=
      TDL->getTypeStoreSizeInBits(Layout.ElemTy))
    return false;

  if (Alignment)
    Layout.VecAlign = Alignment;
  else
    Layout.VecAlign = TDL->getABITypeAlignment(Layout.VecTy);
  Layout.ElemSize = TDL->getTypeStoreSize(Layout.ElemTy);
  return true;
}

// Scalarize two-operand instruction I, using Split(Builder, X, Y, Name)
// to create an instruction like I with operands X and Y and name Name.
template <typename Splitter>
bool Scalarizer::splitBinary(Instruction &I, const Splitter &Split) {
  VectorType *VT = dyn_cast<VectorType>(I.getType());
  if (!VT)
    return false;

  unsigned NumElems = VT->getNumElements();
  IRBuilder<> Builder(I.getParent(), &I);
  Scatterer Op0 = scatter(&I, I.getOperand(0));
  Scatterer Op1 = scatter(&I, I.getOperand(1));
  assert(Op0.size() == NumElems && "Mismatched binary operation");
  assert(Op1.size() == NumElems && "Mismatched binary operation");
  ValueVector Res;
  Res.resize(NumElems);
  for (unsigned Elem = 0; Elem < NumElems; ++Elem)
    Res[Elem] =
        Split(Builder, Op0[Elem], Op1[Elem], I.getName() + ".i" + Twine(Elem));
  gather(&I, Res);
  return true;
}

bool Scalarizer::visitSelectInst(SelectInst &SI) {
  VectorType *VT = dyn_cast<VectorType>(SI.getType());
  if (!VT)
    return false;

  unsigned NumElems = VT->getNumElements();
  IRBuilder<> Builder(SI.getParent(), &SI);
  Scatterer Op1 = scatter(&SI, SI.getOperand(1));
  Scatterer Op2 = scatter(&SI, SI.getOperand(2));
  assert(Op1.size() == NumElems && "Mismatched select");
  assert(Op2.size() == NumElems && "Mismatched select");
  ValueVector Res;
  Res.resize(NumElems);

  if (SI.getOperand(0)->getType()->isVectorTy()) {
    Scatterer Op0 = scatter(&SI, SI.getOperand(0));
    assert(Op0.size() == NumElems && "Mismatched select");
    for (unsigned I = 0; I < NumElems; ++I)
      Res[I] = Builder.CreateSelect(Op0[I], Op1[I], Op2[I],
                                    SI.getName() + ".i" + Twine(I));
  } else {
    Value *Op0 = SI.getOperand(0);
    for (unsigned I = 0; I < NumElems; ++I)
      Res[I] = Builder.CreateSelect(Op0, Op1[I], Op2[I],
                                    SI.getName() + ".i" + Twine(I));
  }
  gather(&SI, Res);
  return true;
}

bool Scalarizer::visitICmpInst(ICmpInst &ICI) {
  return splitBinary(ICI, ICmpSplitter(ICI));
}

bool Scalarizer::visitFCmpInst(FCmpInst &FCI) {
  return splitBinary(FCI, FCmpSplitter(FCI));
}

bool Scalarizer::visitBinaryOperator(BinaryOperator &BO) {
  return splitBinary(BO, BinarySplitter(BO));
}

bool Scalarizer::visitGetElementPtrInst(GetElementPtrInst &GEPI) {
  return splitBinary(GEPI, GEPSplitter());
}

bool Scalarizer::visitCastInst(CastInst &CI) {
  VectorType *VT = dyn_cast<VectorType>(CI.getDestTy());
  if (!VT)
    return false;

  unsigned NumElems = VT->getNumElements();
  IRBuilder<> Builder(CI.getParent(), &CI);
  Scatterer Op0 = scatter(&CI, CI.getOperand(0));
  assert(Op0.size() == NumElems && "Mismatched cast");
  ValueVector Res;
  Res.resize(NumElems);
  for (unsigned I = 0; I < NumElems; ++I)
    Res[I] = Builder.CreateCast(CI.getOpcode(), Op0[I], VT->getElementType(),
                                CI.getName() + ".i" + Twine(I));
  gather(&CI, Res);
  return true;
}

bool Scalarizer::visitBitCastInst(BitCastInst &BCI) {
  VectorType *DstVT = dyn_cast<VectorType>(BCI.getDestTy());
  VectorType *SrcVT = dyn_cast<VectorType>(BCI.getSrcTy());
  if (!DstVT || !SrcVT)
    return false;

  unsigned DstNumElems = DstVT->getNumElements();
  unsigned SrcNumElems = SrcVT->getNumElements();
  IRBuilder<> Builder(BCI.getParent(), &BCI);
  Scatterer Op0 = scatter(&BCI, BCI.getOperand(0));
  ValueVector Res;
  Res.resize(DstNumElems);

  if (DstNumElems == SrcNumElems) {
    for (unsigned I = 0; I < DstNumElems; ++I)
      Res[I] = Builder.CreateBitCast(Op0[I], DstVT->getElementType(),
                                     BCI.getName() + ".i" + Twine(I));
  } else if (DstNumElems > SrcNumElems) {
    // <M x t1> -> <N*M x t2>.  Convert each t1 to <N x t2> and copy the
    // individual elements to the destination.
    unsigned FanOut = DstNumElems / SrcNumElems;
    Type *MidTy = VectorType::get(DstVT->getElementType(), FanOut);
    unsigned ResI = 0;
    for (unsigned Op0I = 0; Op0I < SrcNumElems; ++Op0I) {
      Value *V = Op0[Op0I];
      Instruction *VI;
      // Look through any existing bitcasts before converting to <N x t2>.
      // In the best case, the resulting conversion might be a no-op.
      while ((VI = dyn_cast<Instruction>(V)) &&
             VI->getOpcode() == Instruction::BitCast)
        V = VI->getOperand(0);
      V = Builder.CreateBitCast(V, MidTy, V->getName() + ".cast");
      Scatterer Mid = scatter(&BCI, V);
      for (unsigned MidI = 0; MidI < FanOut; ++MidI)
        Res[ResI++] = Mid[MidI];
    }
  } else {
    // <N*M x t1> -> <M x t2>.  Convert each group of <N x t1> into a t2.
    unsigned FanIn = SrcNumElems / DstNumElems;
    Type *MidTy = VectorType::get(SrcVT->getElementType(), FanIn);
    unsigned Op0I = 0;
    for (unsigned ResI = 0; ResI < DstNumElems; ++ResI) {
      Value *V = UndefValue::get(MidTy);
      for (unsigned MidI = 0; MidI < FanIn; ++MidI)
        V = Builder.CreateInsertElement(V, Op0[Op0I++], Builder.getInt32(MidI),
                                        BCI.getName() + ".i" + Twine(ResI) +
                                            ".upto" + Twine(MidI));
      Res[ResI] = Builder.CreateBitCast(V, DstVT->getElementType(),
                                        BCI.getName() + ".i" + Twine(ResI));
    }
  }
  gather(&BCI, Res);
  return true;
}

bool Scalarizer::visitShuffleVectorInst(ShuffleVectorInst &SVI) {
  VectorType *VT = dyn_cast<VectorType>(SVI.getType());
  if (!VT)
    return false;

  unsigned NumElems = VT->getNumElements();
  Scatterer Op0 = scatter(&SVI, SVI.getOperand(0));
  Scatterer Op1 = scatter(&SVI, SVI.getOperand(1));
  ValueVector Res;
  Res.resize(NumElems);

  for (unsigned I = 0; I < NumElems; ++I) {
    int Selector = SVI.getMaskValue(I);
    if (Selector < 0)
      Res[I] = UndefValue::get(VT->getElementType());
    else if (unsigned(Selector) < Op0.size())
      Res[I] = Op0[Selector];
    else
      Res[I] = Op1[Selector - Op0.size()];
  }
  gather(&SVI, Res);
  return true;
}

bool Scalarizer::visitPHINode(PHINode &PHI) {
  VectorType *VT = dyn_cast<VectorType>(PHI.getType());
  if (!VT)
    return false;

  unsigned NumElems = VT->getNumElements();
  IRBuilder<> Builder(PHI.getParent(), &PHI);
  ValueVector Res;
  Res.resize(NumElems);

  unsigned NumOps = PHI.getNumOperands();
  for (unsigned I = 0; I < NumElems; ++I)
    Res[I] = Builder.CreatePHI(VT->getElementType(), NumOps,
                               PHI.getName() + ".i" + Twine(I));

  for (unsigned I = 0; I < NumOps; ++I) {
    Scatterer Op = scatter(&PHI, PHI.getIncomingValue(I));
    BasicBlock *IncomingBlock = PHI.getIncomingBlock(I);
    for (unsigned J = 0; J < NumElems; ++J)
      cast<PHINode>(Res[J])->addIncoming(Op[J], IncomingBlock);
  }
  gather(&PHI, Res);
  return true;
}

bool Scalarizer::visitLoadInst(LoadInst &LI) {
  if (!ScalarizeLoadStore)
    return false;
  if (!LI.isSimple())
    return false;

  VectorLayout Layout;
  if (!getVectorLayout(LI.getType(), LI.getAlignment(), Layout))
    return false;

  unsigned NumElems = Layout.VecTy->getNumElements();
  IRBuilder<> Builder(LI.getParent(), &LI);
  Scatterer Ptr = scatter(&LI, LI.getPointerOperand());
  ValueVector Res;
  Res.resize(NumElems);

  for (unsigned I = 0; I < NumElems; ++I)
    Res[I] = Builder.CreateAlignedLoad(Ptr[I], Layout.getElemAlign(I),
                                       LI.getName() + ".i" + Twine(I));
  gather(&LI, Res);
  return true;
}

bool Scalarizer::visitStoreInst(StoreInst &SI) {
  if (!ScalarizeLoadStore)
    return false;
  if (!SI.isSimple())
    return false;

  VectorLayout Layout;
  Value *FullValue = SI.getValueOperand();
  if (!getVectorLayout(FullValue->getType(), SI.getAlignment(), Layout))
    return false;

  unsigned NumElems = Layout.VecTy->getNumElements();
  IRBuilder<> Builder(SI.getParent(), &SI);
  Scatterer Ptr = scatter(&SI, SI.getPointerOperand());
  Scatterer Val = scatter(&SI, FullValue);

  ValueVector Stores;
  Stores.resize(NumElems);
  for (unsigned I = 0; I < NumElems; ++I) {
    unsigned Align = Layout.getElemAlign(I);
    Stores[I] = Builder.CreateAlignedStore(Val[I], Ptr[I], Align);
  }
  transferMetadata(&SI, Stores);
  return true;
}

// Delete the instructions that we scalarized.  If a full vector result
// is still needed, recreate it using InsertElements.
bool Scalarizer::finish() {
  if (Gathered.empty())
    return false;
  for (GatherList::iterator GMI = Gathered.begin(), GME = Gathered.end();
       GMI != GME; ++GMI) {
    Instruction *Op = GMI->first;
    ValueVector &CV = *GMI->second;
    if (!Op->use_empty()) {
      // The value is still needed, so recreate it using a series of
      // InsertElements.
      Type *Ty = Op->getType();
      Value *Res = UndefValue::get(Ty);
      unsigned Count = Ty->getVectorNumElements();
      IRBuilder<> Builder(Op->getParent(), Op);
      for (unsigned I = 0; I < Count; ++I)
        Res = Builder.CreateInsertElement(Res, CV[I], Builder.getInt32(I),
                                          Op->getName() + ".upto" + Twine(I));
      Res->takeName(Op);
      Op->replaceAllUsesWith(Res);
    }
    Op->eraseFromParent();
  }
  Gathered.clear();
  Scattered.clear();
  return true;
}

namespace klee {
llvm::FunctionPass *createScalarizerPass() { return new Scalarizer(); }
} // namespace klee

#endif

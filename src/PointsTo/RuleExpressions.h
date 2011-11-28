// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.

#ifndef POINTSTO_RULEEXPRESSIONS_H
#define POINTSTO_RULEEXPRESSIONS_H

#include <map>
#include <iterator>

#include "llvm/Constants.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"

#include "../Languages/LLVM.h"

namespace llvm { namespace ptr {
    template<typename PointsToAlgorithm>
    class Rules;
}}

namespace llvm { namespace ptr {

    struct Expression
    {
        virtual ~Expression() {}
    };

    template<typename SubExpr>
    struct RuleUnaryExpression : public Expression
    {
        typedef SubExpr SubExpression;

        explicit RuleUnaryExpression(SubExpression const sub)
            : arg(sub)
        {}

        SubExpression getArgument() const
        { return arg; }


        SubExpression arg;
    };

    template<typename SubExpr1, typename SubExpr2>
    struct RuleBinaryExpression : public Expression
    {
        typedef SubExpr1 SubExpression1;
        typedef SubExpr2 SubExpression2;

        explicit RuleBinaryExpression(SubExpression1 const sub1,
                                      SubExpression2 const sub2)
            : arg1(sub1)
            , arg2(sub2)
        {}

        SubExpression1 getArgument1() const
        { return arg1; }

        SubExpression2 getArgument2() const
        { return arg2; }


        SubExpression1 arg1;
        SubExpression2 arg2;
    };

    template<typename MemLoc>
    struct VARIABLE : public RuleUnaryExpression<MemLoc> {
        VARIABLE(MemLoc const ml) : RuleUnaryExpression<MemLoc>(ml) {}
    };

    template<typename MemLoc>
    struct ALLOC : public RuleUnaryExpression<MemLoc> {
        ALLOC(MemLoc const ml) : RuleUnaryExpression<MemLoc>(ml) {}
    };

    template<typename MemLoc>
    struct DEALLOC : public RuleUnaryExpression<MemLoc> {
        DEALLOC(MemLoc const ml) : RuleUnaryExpression<MemLoc>(ml) {}
    };

    template<typename MemLoc>
    struct NULLPTR : public RuleUnaryExpression<MemLoc> {
        NULLPTR(MemLoc const ml) : RuleUnaryExpression<MemLoc>(ml) {}
    };

    template<typename SubExpr>
    struct REFERENCE : public RuleUnaryExpression<SubExpr> {
        REFERENCE(SubExpr const sub) : RuleUnaryExpression<SubExpr>(sub) {}
    };

    template<typename SubExpr>
    struct DEREFERENCE : public RuleUnaryExpression<SubExpr> {
        DEREFERENCE(SubExpr const sub) : RuleUnaryExpression<SubExpr>(sub) {}
    };

    template<typename LSubExpr, typename RSubExpr>
    struct ASSIGNMENT : public RuleBinaryExpression<LSubExpr,RSubExpr> {
        ASSIGNMENT(LSubExpr const lsub, RSubExpr const rsub)
            : RuleBinaryExpression<LSubExpr,RSubExpr>(lsub,rsub) {}
    };

    template<typename ExprSort>
    struct RuleExpression
    {
        typedef ExprSort Sort;

        explicit RuleExpression(Sort const& s)
            : sort(s)
        {}

        Sort getSort() const
        { return sort; }

        template<typename RSort>
        RuleExpression< ASSIGNMENT<Sort,RSort> >
        operator=(RuleExpression<RSort> const& r) const
        { return makeAssignment(r); }

        template<typename RSort>
        RuleExpression< ASSIGNMENT<Sort,RSort> >
        operator=(RuleExpression<RSort> const& r)
        { return makeAssignment(r); }

        RuleExpression< ASSIGNMENT<Sort,Sort> >
        operator=(RuleExpression<Sort> const& r)
        { return makeAssignment(r); }

        RuleExpression< REFERENCE<Sort> > operator&() const
        {
            return RuleExpression< REFERENCE<Sort> >(REFERENCE<Sort>(sort));
        }

        RuleExpression< DEREFERENCE<Sort> > operator*() const
        {
            return RuleExpression< DEREFERENCE<Sort> >(DEREFERENCE<Sort>(sort));
        }
    private:
        template<typename RSort>
        RuleExpression< ASSIGNMENT<Sort,RSort> >
        makeAssignment(RuleExpression<RSort> const& r)
        {
            return RuleExpression< ASSIGNMENT<Sort,RSort> >(
                        ASSIGNMENT<Sort,RSort>(sort,r.getSort()));
        }

        Sort sort;
    };

    template<typename MemLoc>
    RuleExpression< VARIABLE<MemLoc> > ruleVar(MemLoc const ml)
    {
        return RuleExpression< VARIABLE<MemLoc> >(VARIABLE<MemLoc>(ml));
    }

    template<typename MemLoc>
    RuleExpression< ALLOC<MemLoc> > ruleAllocSite(MemLoc const ml)
    {
        return RuleExpression< ALLOC<MemLoc> >(ALLOC<MemLoc>(ml));
    }

    template<typename MemLoc>
    RuleExpression< DEALLOC<MemLoc> > ruleDeallocSite(MemLoc const ml)
    {
        return RuleExpression< DEALLOC<MemLoc> >(DEALLOC<MemLoc>(ml));
    }

    template<typename MemLoc>
    RuleExpression< NULLPTR<MemLoc> > ruleNull(MemLoc const ml)
    {
        return RuleExpression< NULLPTR<MemLoc> >(NULLPTR<MemLoc>(ml));
    }

}}

namespace llvm { namespace ptr {

    enum RuleCodeType
    {
        RCT_UNKNOWN = 0,
        RCT_VAR_ASGN_ALLOC,
        RCT_VAR_ASGN_NULL,
        RCT_VAR_ASGN_VAR,
        RCT_VAR_ASGN_REF_VAR,
        RCT_VAR_ASGN_DREF_VAR,
        RCT_DREF_VAR_ASGN_NULL,
        RCT_DREF_VAR_ASGN_VAR,
        RCT_DREF_VAR_ASGN_REF_VAR,
        RCT_DREF_VAR_ASGN_DREF_VAR,
        RCT_DEALLOC,
    };

    struct RuleCode
    {
        typedef const llvm::Value *MemoryLocation;

        RuleCode()
            : type(RCT_UNKNOWN)
        {}

        RuleCode(ASSIGNMENT<VARIABLE<MemoryLocation>,
                            ALLOC<MemoryLocation> > const& E)
            : type(RCT_VAR_ASGN_ALLOC)
            , lvalue(E.getArgument1().getArgument())
            , rvalue(E.getArgument2().getArgument())
        {}

        RuleCode(ASSIGNMENT<VARIABLE<MemoryLocation>,
                            NULLPTR<MemoryLocation> > const& E)
            : type(RCT_VAR_ASGN_NULL)
            , lvalue(E.getArgument1().getArgument())
            , rvalue(E.getArgument2().getArgument())
        {}

        RuleCode(ASSIGNMENT<VARIABLE<MemoryLocation>,
                            VARIABLE<MemoryLocation> > const& E)
            : type(RCT_VAR_ASGN_VAR)
            , lvalue(E.getArgument1().getArgument())
            , rvalue(E.getArgument2().getArgument())
        {}

        RuleCode(ASSIGNMENT<VARIABLE<MemoryLocation>,
                            REFERENCE<VARIABLE<MemoryLocation> > > const& E)
            : type(RCT_VAR_ASGN_REF_VAR)
            , lvalue(E.getArgument1().getArgument())
            , rvalue(E.getArgument2().getArgument().getArgument())
        {}

        RuleCode(ASSIGNMENT<VARIABLE<MemoryLocation>,
                            DEREFERENCE<VARIABLE<MemoryLocation> > > const& E)
            : type(RCT_VAR_ASGN_DREF_VAR)
            , lvalue(E.getArgument1().getArgument())
            , rvalue(E.getArgument2().getArgument().getArgument())
        {}

        RuleCode(ASSIGNMENT<DEREFERENCE<VARIABLE<MemoryLocation> >,
                            NULLPTR<MemoryLocation> > const& E)
            : type(RCT_DREF_VAR_ASGN_NULL)
            , lvalue(E.getArgument1().getArgument().getArgument())
            , rvalue(E.getArgument2().getArgument())
        {}

        RuleCode(ASSIGNMENT<DEREFERENCE<VARIABLE<MemoryLocation> >,
                            VARIABLE<MemoryLocation> > const& E)
            : type(RCT_DREF_VAR_ASGN_VAR)
            , lvalue(E.getArgument1().getArgument().getArgument())
            , rvalue(E.getArgument2().getArgument())
        {}

        RuleCode(ASSIGNMENT<DEREFERENCE<VARIABLE<MemoryLocation> >,
                            REFERENCE<VARIABLE<MemoryLocation> > > const& E)
            : type(RCT_DREF_VAR_ASGN_REF_VAR)
            , lvalue(E.getArgument1().getArgument().getArgument())
            , rvalue(E.getArgument2().getArgument().getArgument())
        {}

        RuleCode(ASSIGNMENT<DEREFERENCE<VARIABLE<MemoryLocation> >,
                            DEREFERENCE<VARIABLE<MemoryLocation> > > const& E)
            : type(RCT_DREF_VAR_ASGN_DREF_VAR)
            , lvalue(E.getArgument1().getArgument().getArgument())
            , rvalue(E.getArgument2().getArgument().getArgument())
        {}

        RuleCode(DEALLOC<MemoryLocation> const& E)
            : type(RCT_DEALLOC)
            , lvalue(E.getArgument())
            , rvalue()
        {}

        RuleCodeType getType() const { return type; }
        MemoryLocation const& getLvalue() const { return lvalue; }
        MemoryLocation const& getRvalue() const { return rvalue; }
        MemoryLocation const& getValue() const { return getLvalue(); }

    private:
        RuleCodeType type;
        MemoryLocation lvalue;
        MemoryLocation rvalue;
    };

    template<typename ExprSort>
    RuleCode ruleCode(RuleExpression<ExprSort> const& E)
    {
        return RuleCode(E.getSort());
    }

    template<typename PointsToAlgorithm>
    void getRulesOfCommand(RuleCode const& RC,
        Rules<PointsToAlgorithm>& R)
    {
        switch (RC.getType())
        {
            case RCT_VAR_ASGN_ALLOC:
                R.insert(ruleVar(RC.getLvalue())=ruleAllocSite(RC.getRvalue()));
                break;
            case RCT_VAR_ASGN_NULL:
                R.insert(ruleVar(RC.getLvalue()) = ruleNull(RC.getRvalue()));
                break;
            case RCT_VAR_ASGN_VAR:
                R.insert(ruleVar(RC.getLvalue()) = ruleVar(RC.getRvalue()));
                break;
            case RCT_VAR_ASGN_REF_VAR:
                R.insert(ruleVar(RC.getLvalue()) = &ruleVar(RC.getRvalue()));
                break;
            case RCT_VAR_ASGN_DREF_VAR:
                R.insert(ruleVar(RC.getLvalue()) = *ruleVar(RC.getRvalue()));
                break;
            case RCT_DREF_VAR_ASGN_NULL:
                R.insert(*ruleVar(RC.getLvalue()) = ruleNull(RC.getRvalue()));
                break;
            case RCT_DREF_VAR_ASGN_VAR:
                R.insert(*ruleVar(RC.getLvalue()) = ruleVar(RC.getRvalue()));
                break;
            case RCT_DREF_VAR_ASGN_REF_VAR:
                R.insert(*ruleVar(RC.getLvalue()) = &ruleVar(RC.getRvalue()));
                break;
            case RCT_DREF_VAR_ASGN_DREF_VAR:
                R.insert(*ruleVar(RC.getLvalue()) = *ruleVar(RC.getRvalue()));
                break;
            case RCT_DEALLOC:
                R.insert(ruleDeallocSite(RC.getValue()));
                break;
            default:
                break;
        }
    }

}}

namespace llvm { namespace ptr {

    template<typename OutputStream, typename ExprSort>
    OutputStream& dump(OutputStream& ostr, RuleExpression<ExprSort> const& E)
    {
        return dump(ostr,E.getSort());
    }

    template<typename OutputStream, typename LSubExpr, typename RSubExpr>
    OutputStream& dump(OutputStream& ostr,
                       ASSIGNMENT<LSubExpr,RSubExpr> const& E)
    {
        dump(ostr,E.getArgument1());
        ostr << " = ";
        dump(ostr,E.getArgument2());
        return ostr;
    }

    template<typename OutputStream, typename SubExpr>
    OutputStream& dump(OutputStream& ostr, REFERENCE<SubExpr> const& E)
    {
        ostr << '&';
        dump(ostr,E.getArgument());
        return ostr;
    }

    template<typename OutputStream, typename SubExpr>
    OutputStream& dump(OutputStream& ostr, DEREFERENCE<SubExpr> const& E)
    {
        ostr << '*';
        dump(ostr,E.getArgument());
        return ostr;
    }
#if 0
    template<typename OutputStream, typename MemLoc>
    OutputStream& dump(OutputStream& ostr, VARIABLE<MemLoc> const& E)
    {
        using monty::codespy::dump;
        dump(ostr,E.getArgument());
        return ostr;
    }

    template<typename OutputStream, typename MemLoc>
    OutputStream& dump(OutputStream& ostr, ALLOC<MemLoc> const& E)
    {
        using monty::codespy::dump;
        ostr << "ALLOC@";
        dump(ostr,E.getArgument());
        return ostr;
    }

    template<typename OutputStream, typename MemLoc>
    OutputStream& dump(OutputStream& ostr, DEALLOC<MemLoc> const& E)
    {
        using monty::codespy::dump;
        ostr << "DEALLOC@";
        dump(ostr,E.getArgument());
        return ostr;
    }

    template<typename OutputStream, typename MemLoc>
    OutputStream& dump(OutputStream& ostr, NULLPTR<MemLoc> const& E)
    {
        using monty::codespy::dump;
        ostr << "NULL@";
        dump(ostr,E.getArgument());
        return ostr;
    }

    template<typename OutputStream, typename MemLoc>
    OutputStream& dump(OutputStream& ostr, RuleCode<MemLoc> const& RC)
    {
        MONTY_TMPROF_BLOCK_LVL1();

        switch (RC.getType())
        {
            case RCT_VAR_ASGN_ALLOC:
                dump(ostr,ruleVar(RC.getLvalue())=ruleAllocSite(RC.getRvalue()));
                break;
            case RCT_VAR_ASGN_NULL:
                dump(ostr,ruleVar(RC.getLvalue())=ruleNull(RC.getRvalue()));
                break;
            case RCT_VAR_ASGN_VAR:
                dump(ostr,ruleVar(RC.getLvalue()) = ruleVar(RC.getRvalue()));
                break;
            case RCT_VAR_ASGN_REF_VAR:
                dump(ostr,ruleVar(RC.getLvalue()) = &ruleVar(RC.getRvalue()));
                break;
            case RCT_VAR_ASGN_DREF_VAR:
                dump(ostr,ruleVar(RC.getLvalue()) = *ruleVar(RC.getRvalue()));
                break;
            case RCT_DREF_VAR_ASGN_NULL:
                dump(ostr,*ruleVar(RC.getLvalue()) = ruleNull(RC.getRvalue()));
                break;
            case RCT_DREF_VAR_ASGN_VAR:
                dump(ostr,*ruleVar(RC.getLvalue()) = ruleVar(RC.getRvalue()));
                break;
            case RCT_DREF_VAR_ASGN_REF_VAR:
                dump(ostr,*ruleVar(RC.getLvalue()) = &ruleVar(RC.getRvalue()));
                break;
            case RCT_DREF_VAR_ASGN_DREF_VAR:
                dump(ostr,*ruleVar(RC.getLvalue()) = *ruleVar(RC.getRvalue()));
                break;
            case RCT_DEALLOC:
                dump(ostr,ruleDeallocSite(RC.getValue()));
                break;
            default:
                ostr << "<UNKNOWN_RULE_CODE>";
                break;
        }
        return ostr;
    }
#endif
}}

namespace llvm { namespace ptr { namespace detail {

    typedef std::multimap<llvm::FunctionType const*,llvm::Function const*>
            FunctionsMap;

    typedef std::multimap<llvm::FunctionType const*,llvm::CallInst const*>
            CallsMap;

    void buildCallMaps(llvm::Module const& M, FunctionsMap& F, CallsMap& C);
    RuleCode argPassRuleCode(llvm::Value const* const l,
                                                 llvm::Value const* const r);

  template<typename OutIterator>
  void toRuleCode(const Value *V, OutIterator out) {
    if (const llvm::Instruction *I = llvm::dyn_cast<llvm::Instruction>(V)) {
      if (const llvm::LoadInst *LI = llvm::dyn_cast<llvm::LoadInst>(I)) {
        const llvm::Value *op = elimConstExpr(LI->getPointerOperand());
        if (hasExtraReference(op))
          *out++ = ruleCode(ruleVar(V) = ruleVar(op));
        else
          *out++ = ruleCode(ruleVar(V) = *ruleVar(op));
            } else if (const llvm::StoreInst *SI =
                       llvm::dyn_cast<llvm::StoreInst>(I)) {
        const llvm::Value *l = elimConstExpr(SI->getPointerOperand());
        const llvm::Value *r = elimConstExpr(SI->getValueOperand());
        if (!hasExtraReference(l)) {
          if (hasExtraReference(r))
            *out++ = ruleCode(*ruleVar(l) = &ruleVar(r));
          else {
            if (isa<ConstantPointerNull>(r))
              *out++ = ruleCode(*ruleVar(l) = ruleNull(r));
            else
              *out++ = ruleCode(*ruleVar(l) = ruleVar(r));
          }
        } else {
          if (hasExtraReference(r))
            *out++ = ruleCode(ruleVar(l) = &ruleVar(r));
          else {
            if (isa<ConstantPointerNull>(r))
              *out++ = ruleCode(ruleVar(l) = ruleNull(r));
            else
              *out++ = ruleCode(ruleVar(l) = ruleVar(r));
          }
        }
            } else if (const llvm::BitCastInst *BCI =
                       llvm::dyn_cast<llvm::BitCastInst>(I)) {
          const llvm::Value *op = elimConstExpr(BCI->getOperand(0));
          if (hasExtraReference(op))
            *out++ = ruleCode(ruleVar(V) = &ruleVar(op));
          else
            *out++ = ruleCode(ruleVar(V) = ruleVar(op));
            } else if (const llvm::GetElementPtrInst *gep =
                       llvm::dyn_cast<llvm::GetElementPtrInst>(I)) {
          const llvm::Value *op = elimConstExpr(gep->getPointerOperand());
          if (hasExtraReference(op))
            *out++ = ruleCode(ruleVar(V) = &ruleVar(op));
          else
            *out++ = ruleCode(ruleVar(V) = ruleVar(op));
            } else if (const llvm::CallInst *C =
                       llvm::dyn_cast<llvm::CallInst>(I)) {
        if (isInlineAssembly(C)) {
        } if (isMemoryAllocation(C->getCalledValue()))
          *out++ = ruleCode(ruleVar(V) = ruleAllocSite(V));
        else if (isMemoryDeallocation(C->getCalledValue()))
          errs() << "KOKO\n";
        //                    *out++ = ruleCode(ruleDeallocSite(V));
        else if (isMemoryCopy(C->getCalledValue()) ||
            isMemoryMove(C->getCalledValue())) {
          const llvm::Value *l = elimConstExpr(C->getArgOperand(0));
          const llvm::Value *r = elimConstExpr(C->getArgOperand(1));
          *out++ = ruleCode(*ruleVar(l) = *ruleVar(r));
        }
            } else if (const llvm::PHINode *PHI = llvm::dyn_cast<llvm::PHINode>(I)) {
              unsigned int i, n = PHI->getNumIncomingValues();
              for (i = 0; i < n; ++i) {
                const llvm::Value *r = PHI->getIncomingValue(i);
                if (llvm::isa<llvm::ConstantPointerNull>(r))
                    *out++ = ruleCode(ruleVar(V) = ruleNull(r));
                else
                    *out++ = ruleCode(ruleVar(V) = ruleVar(r));
              }
            } else if (const llvm::ExtractValueInst *EV =
                       llvm::dyn_cast<llvm::ExtractValueInst>(I)) {
        // TODO: Instruction 'ExtractValueIns' has not been tested yet!

        const llvm::Value *op = EV->getAggregateOperand();
        assert(!hasExtraReference(op) && "Agregate operand must "
               "be a value and not a pointer.");
        *out++ = ruleCode(ruleVar(V) = ruleVar(op));
            } else if (const llvm::InsertValueInst *IV =
                       llvm::dyn_cast<llvm::InsertValueInst>(I)) {
        // TODO: Instruction 'InsertValueInst' has not been tested yet!

        const llvm::Value *l = IV->getAggregateOperand();
        assert(!hasExtraReference(l) && "Agregate operand must "
               "be a value and not a pointer.");
        const llvm::Value *r = IV->getInsertedValueOperand();
        if (hasExtraReference(r))
          *out++ = ruleCode(ruleVar(l) = &ruleVar(r));
        else {
          if (isa<ConstantPointerNull>(r))
            *out++ = ruleCode(ruleVar(l) = ruleNull(r));
          else
            *out++ = ruleCode(ruleVar(l) = ruleVar(r));
        }
      } else if (llvm::isa<llvm::IntToPtrInst>(I)) {
        *out++ = ruleCode(ruleVar(V) = &ruleVar(getUndefValue(I->getContext())));
      }
    } else if (const llvm::GlobalVariable *GV =
               llvm::dyn_cast<llvm::GlobalVariable>(V)) {
      const llvm::Value *op = GV->getInitializer();
      *out++ = ruleCode(ruleVar(V) = &ruleVar(op));
    }
  }


    template<typename OutIterator>
    void collectCallRuleCodes(llvm::CallInst const* const c,
                              const llvm::Function *f, OutIterator out) {
	assert(!isInlineAssembly(c) && "Inline assembly is not supported!");
        if (memoryManStuff(f) && !llvm::isMemoryAllocation(f))
            return;
        if (llvm::isMemoryAllocation(f))
        {
            llvm::Value const* const V = c;
            *out++ = ruleCode(ruleVar(V) = ruleAllocSite(V));
        }
        else
        {
            llvm::Function::const_arg_iterator fit = f->arg_begin();
            for (std::size_t i = 0; fit != f->arg_end(); ++fit, ++i)
                if (llvm::isPointerValue(&*fit))
                    *out++ = detail::argPassRuleCode(&*fit,
                                            elimConstExpr(c->getOperand(i)));
            if (f->isDeclaration() && f->getReturnType()->isPointerTy()) {
              const llvm::Value *l = c;
              const llvm::Value *r =
                  UndefValue::get(getPointedType(f->getReturnType()));
              *out++ = ruleCode(ruleVar(l) = &ruleVar(r));
            }
        }
    }

    template<typename OutIterator>
    void collectCallRuleCodes(llvm::CallInst const* const c,
                              FunctionsMap::const_iterator b,
                              FunctionsMap::const_iterator const e,
                              OutIterator out)
    {
        if (llvm::Function const* f = c->getCalledFunction())
            collectCallRuleCodes(c,f,out);
        else
            for ( ; b != e; ++b)
                collectCallRuleCodes(c,b->second,out);
    }

    template<typename OutIterator>
    void collectReturnRuleCodes(llvm::ReturnInst const* const r,
                                CallsMap::const_iterator b,
                                CallsMap::const_iterator const e,
                                OutIterator out)
    {
        if (r->getNumOperands() == 0 ||
                !llvm::isPointerValue(r->getOperand(0)))
            return;
        llvm::Function const* const f = r->getParent()->getParent();
        for ( ; b != e; ++b)
            if (llvm::Function const* g = b->second->getCalledFunction())
            {
                if (f == g)
                    *out++ =detail::argPassRuleCode(b->second,r->getOperand(0));
            }
            else
                *out++ = detail::argPassRuleCode(b->second,r->getOperand(0));
    }

}}}

#endif

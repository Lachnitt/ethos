#include "type_checker.h"

#include <iostream>
#include <set>
#include <unordered_map>

#include "base/check.h"
#include "base/output.h"
#include "state.h"
#include "parser.h"
#include "literal.h"

namespace alfc {
  
std::ostream& operator<<(std::ostream& out, const Ctx& c)
{
  out << "[";
  bool firstTime = true;
  for (const std::pair<const Expr, Expr>& cc : c)
  {
    if (firstTime)
    {
      firstTime = false;
    }
    else
    {
      out << ", ";
    }
    out << cc.first << " -> " << cc.second;
  }
  out << "]";
  return out;
}

TypeChecker::TypeChecker(State& s) : d_state(s)
{
  d_literalKinds = { Kind::BOOLEAN, Kind::NUMERAL,  Kind::DECIMAL, Kind::HEXADECIMAL, Kind::BINARY, Kind::STRING };
  // initialize literal kinds 
  for (Kind k : d_literalKinds)
  {
    d_literalTypeRules[k] = nullptr;
  }
}

TypeChecker::~TypeChecker()
{
}

void TypeChecker::setLiteralTypeRule(Kind k, const Expr& t)
{
  std::map<Kind, Expr>::iterator it = d_literalTypeRules.find(k);
  if (it==d_literalTypeRules.end())
  {
    std::stringstream ss;
    ALFC_FATAL() << "TypeChecker::setTypeRule: cannot set type rule for kind "
                 << k;
  }
  else if (it->second!=nullptr && it->second!=t)
  {
    std::stringstream ss;
    ALFC_FATAL() << "TypeChecker::setTypeRule: cannot set type rule for kind "
                 << k << " to " << t << ", since its type was already set to "
                 << it->second;
  }
  it->second = t;
}

Expr TypeChecker::getOrSetLiteralTypeRule(Kind k)
{
  std::map<Kind, Expr>::iterator it = d_literalTypeRules.find(k);
  if (it==d_literalTypeRules.end())
  {
    std::stringstream ss;
    ALFC_FATAL() << "TypeChecker::getOrSetLiteralTypeRule: cannot get type rule for kind "
                 << k;
  }
  if (it->second==nullptr)
  {
    // If no type rule, assign the type rule to the builtin type
    Expr t = d_state.mkBuiltinType(k);
    d_literalTypeRules[k] = t;
    return t;
  }
  return it->second;
}

void TypeChecker::defineProgram(const Expr& v, const Expr& prog)
{
  d_programs[v] = prog;
}

bool TypeChecker::hasProgram(const Expr& v) const
{
  return d_programs.find(v)!=d_programs.end();
}

const Expr& TypeChecker::getType(Expr& e, std::ostream* out)
{
  std::unordered_set<Expr> visited;
  std::vector<Expr> toVisit;
  toVisit.push_back(e);
  Expr cur;
  do
  {
    cur = toVisit.back();
    if (cur->d_type!=nullptr)
    {
      // already computed type
      toVisit.pop_back();
      continue;
    }
    if (visited.find(cur)==visited.end())
    {
      visited.insert(cur);
      toVisit.insert(toVisit.end(), cur->d_children.begin(), cur->d_children.end());
    }
    else
    {
      //std::cout << "Type check " << cur << std::endl;
      cur->d_type = getTypeInternal(cur, out);
      if (cur->d_type==nullptr)
      {
        // any subterm causes type checking to fail
        Trace("type_checker") << "TYPE " << cur << " : [FAIL]" << std::endl;
        Assert(e->d_type == nullptr);
        return e->d_type;
      }
      Trace("type_checker")
          << "TYPE " << cur << " : " << cur->d_type << std::endl;
      // std::cout << "...return" << std::endl;
      toVisit.pop_back();
    }
  }while (!toVisit.empty());
  return e->d_type;
}

bool TypeChecker::checkArity(Kind k, size_t nargs)
{
  // check arities
  switch(k)
  {
    case Kind::NIL:
      return nargs==0;
    case Kind::EVAL_IS_EQ:
    case Kind::EVAL_TO_LIST:
    case Kind::EVAL_FROM_LIST:
    case Kind::EVAL_AND:
    case Kind::EVAL_OR:
    case Kind::EVAL_ADD:
    case Kind::EVAL_MUL:
    case Kind::EVAL_INT_DIV:
    case Kind::EVAL_RAT_DIV:
    case Kind::EVAL_CONCAT:
    case Kind::EVAL_TO_BV:
      return nargs==2;
    case Kind::PROOF_TYPE:
    case Kind::EVAL_NOT:
    case Kind::EVAL_NEG:
    case Kind::EVAL_IS_NEG:
    case Kind::EVAL_IS_ZERO:
    case Kind::EVAL_LENGTH:
    case Kind::EVAL_TO_INT:
    case Kind::EVAL_TO_RAT:
    case Kind::EVAL_TO_STRING:
      return nargs==1;
    case Kind::EVAL_REQUIRES:
    case Kind::EVAL_IF_THEN_ELSE:
    case Kind::EVAL_CONS:
    case Kind::EVAL_APPEND:
    case Kind::EVAL_EXTRACT:
      return nargs==3;
    default:break;
  }  
  return true;
}

Expr TypeChecker::getTypeInternal(Expr& e, std::ostream* out)
{
  Kind k = e->getKind();
  if (!checkArity(k, e->getNumChildren()))
  {
    (*out) << "Incorrect arity for " << k;
    return nullptr;
  }
  switch(k)
  {
    case Kind::APPLY:
    {
      return getTypeApp(e->d_children, out);
    }
    case Kind::LAMBDA:
    {
      std::vector<Expr> args;
      std::vector<Expr>& vars = e->d_children[0]->d_children;
      for (Expr& c : vars)
      {
        Assert(c->d_type != nullptr);
        args.push_back( c->d_type);
      }
      Expr ret = e->d_children[1]->d_type;
      Assert(ret != nullptr);
      return d_state.mkFunctionType(args, ret);
    }
    case Kind::NIL:
    case Kind::FAIL:
      // nil is its own type
      return e;
    case Kind::TYPE:
    case Kind::ABSTRACT_TYPE:
    case Kind::BOOL_TYPE:
    case Kind::FUNCTION_TYPE:
      return d_state.mkType();
    case Kind::PROOF_TYPE:
    {
      const Expr& ctype = e->d_children[0]->d_type;
      Assert(ctype != nullptr);
      if (ctype->getKind()!=Kind::BOOL_TYPE)
      {
        if (out)
        {
          (*out) << "Non-Bool for argument of Proof";
        }
        return nullptr;
      }
    }
      return d_state.mkType();
    case Kind::QUOTE_TYPE:
      // anything can be quoted
      return d_state.mkType();
    case Kind::TUPLE:
      // not typed
      return d_state.mkAbstractType();
    case Kind::BOOLEAN:
      // note that Bool is builtin
      return d_state.mkBoolType();
    case Kind::NUMERAL:
    case Kind::DECIMAL:
    case Kind::HEXADECIMAL:
    case Kind::BINARY:
    case Kind::STRING:
    {
      // use the literal type rule
      Expr ret = getOrSetLiteralTypeRule(k);
      // it may involve the "self" parameter
      if (!ret->isGround())
      {
        Ctx ctx;
        ctx[d_state.mkSelf()] = e;
        return evaluate(ret, ctx);
      }
      return ret;
    }
      break;
    default:
      // if a literal operator, consult auxiliary method
      if (isLiteralOp(k))
      {
        std::vector<Expr> ctypes;
        std::vector<Expr>& children = e->d_children;
        for (Expr& c : children)
        {
          ctypes.push_back(c->d_type);
        }
        return getLiteralOpType(k, ctypes, out);
      }
      break;
  }
  if (out)
  {
    (*out) << "Unknown kind " << k;
  }
  return nullptr;
}

Expr TypeChecker::getTypeApp(std::vector<Expr>& children, std::ostream* out)
{
  Assert (!children.empty());
  Expr& hd = children[0];
  Expr hdType = hd->d_type;
  Assert(hdType != nullptr);
  if (hdType->getKind()!=Kind::FUNCTION_TYPE)
  {
    // non-function at head
    if (out)
    {
      (*out) << "Non-function " << hd << " as head of APPLY";
    }
    return nullptr;
  }
  std::vector<Expr>& hdtypes = hdType->d_children;
  std::vector<Expr> ctypes;
  if (hdtypes.size() != children.size())
  {
    // incorrect arity
    if (out)
    {
      (*out) << "Incorrect arity for " << hd << ", #argTypes=" << hdtypes.size()
              << " #children=" << children.size();
    }
    return nullptr;
  }
  for (size_t i=1, nchild=children.size(); i<nchild; i++)
  {
    Assert (children[i]!=nullptr);
    // if the argument type is (Quote t), then we implicitly upcast
    // the argument c to (quote c). This is equivalent to matching
    // c to t directly, hence we take the child itself and not its
    // type.
    Expr arg;
    if (hdtypes[i-1]->getKind()==Kind::QUOTE_TYPE)
    {
      arg = evaluate(children[i]);
    }
    else
    {
      Assert (children[i]->d_type!=nullptr);
      arg = children[i]->d_type;
    }
    ctypes.emplace_back(arg);
  }
  // if compiled, run the compiled version of the type checker
  if (hdType->isCompiled())
  {
    Trace("type_checker") << "RUN type check " << hdType << std::endl;
    return run_getTypeInternal(hdType, ctypes, out);
  }
  Ctx ctx;
  std::set<std::pair<Expr, Expr>> visited;
  for (size_t i=0, nchild=ctypes.size(); i<nchild; i++)
  {
    Assert(ctypes[i] != nullptr);
    // matching, update context
    Expr hdt = hdtypes[i];
    // if the argument is (Quote t), we match on its argument,
    // which along with how ctypes[i] is the argument itself, has the effect
    // of an implicit upcast.
    hdt = hdt->getKind()==Kind::QUOTE_TYPE ? hdt->getChildren()[0] : hdt;
    if (!match(hdt, ctypes[i], ctx, visited))
    {
      if (out)
      {
        (*out) << "Unexpected argument type " << i << " of " << hd << std::endl;
        (*out) << "  LHS " << evaluate(hdtypes[i], ctx) << ", from " << hdtypes[i] << std::endl;
        (*out) << "  RHS " << ctypes[i] << std::endl;
      }
      return nullptr;
    }
  }
  // evaluate in the matched context
  return evaluate(hdtypes.back(), ctx);
}

bool TypeChecker::match(const Expr& a, const Expr& b, Ctx& ctx)
{
  std::set<std::pair<Expr, Expr>> visited;
  return match(a, b, ctx, visited);
}

bool TypeChecker::match(const Expr& a, const Expr& b, Ctx& ctx, std::set<std::pair<Expr, Expr>>& visited)
{
  std::set<std::pair<Expr, Expr>>::iterator it;
  std::map<Expr, Expr>::iterator ctxIt;

  std::vector<std::pair<Expr, Expr>> stack;
  stack.emplace_back(a, b);
  std::pair<Expr, Expr> curr;

  while (!stack.empty())
  {
    curr = stack.back();
    stack.pop_back();
    if (curr.first == curr.second)
    {
      // holds trivially
      continue;
    }
    it = visited.find(curr);
    if (it != visited.end())
    {
      // already processed
      continue;
    }
    visited.insert(curr);
    if (curr.first->getNumChildren() == 0)
    {
      // if the two subterms are not equal and the first one is a bound
      // variable...
      if (curr.first->getKind() == Kind::PARAM)
      {
        // and we have not seen this variable before...
        ctxIt = ctx.find(curr.first);
        if (ctxIt == ctx.cend())
        {
          // TODO: ensure types are the same?
          // add the two subterms to `sub`
          ctx.emplace(curr.first, curr.second);
        }
        else if (ctxIt->second!=curr.second)
        {
          // if we saw this variable before, make sure that (now and before) it
          // maps to the same subterm
          return false;
        }
      }
      else
      {
        // the two subterms are not equal
        return false;
      }
    }
    else
    {
      // if the two subterms are not equal, make sure that their operators are
      // equal
      if (curr.first->getNumChildren() != curr.second->getNumChildren()
          || curr.first->getKind() != curr.second->getKind())
      {
        return false;
      }
      // recurse on children
      for (size_t i = 0, n = curr.first->getNumChildren(); i < n; ++i)
      {
        stack.emplace_back((*curr.first)[i], (*curr.second)[i]);
      }
    }
  }
  return true;
}

Expr TypeChecker::evaluate(Expr& e)
{
  Ctx ctx;
  return evaluate(e, ctx);
}

Expr TypeChecker::evaluate(Expr& e, Ctx& ctx)
{
  Assert (e!=nullptr);
  std::unordered_map<Expr, Expr>::iterator it;
  Ctx::iterator itc;
  
  std::vector<std::unordered_map<Expr, Expr>> visiteds;
  std::vector<Ctx> ctxs;
  std::vector<std::vector<Expr>> visits;
  std::vector<ExprTrie*> ets;
  Expr evaluated;
  Expr cur;
  Expr init;
  visiteds.emplace_back();
  ctxs.emplace_back(ctx);
  visits.emplace_back(std::vector<Expr>{e});
  Kind ck;
  while (!visits.empty())
  {
    std::unordered_map<Expr, Expr>& visited = visiteds.back();
    std::vector<Expr>& visit = visits.back();
    Ctx& cctx = ctxs.back();
    init = visit[0];
    while (!visit.empty())
    {
      cur = visit.back();
      Trace("type_checker_debug") << "visit " << cur << " " << cctx << ", depth=" << visits.size() << std::endl;
      // the term will stay the same if it is not evaluatable and either it
      // is ground, or the context is empty.
      if (!cur->isEvaluatable() && (cur->isGround() || cctx.empty()))
      {
        //std::cout << "...shortcut " << cur << std::endl;
        visited[cur] = cur;
        visit.pop_back();
        continue;
      }
      if (cur->getKind()==Kind::PARAM)
      {
        // might be in context
        itc = cctx.find(cur);
        if (itc!=cctx.end())
        {
          visited[cur] = itc->second;
          visit.pop_back();
          continue;
        }
        visited[cur] = cur;
        visit.pop_back();
        continue;
        // NOTE: this could be an error or warning, variable not filled?
        //std::cout << "WARNING: unfilled variable " << cur << std::endl;
      }
      ck = cur->getKind();
      std::vector<Expr>& children = cur->d_children;
      it = visited.find(cur);
      if (it == visited.end())
      {
        // if it is compiled, we run its evaluation here
        if (cur->isCompiled())
        {
          Trace("type_checker") << "RUN evaluate " << cur << std::endl;
          Expr retev = run_evaluate(cur, cctx);
          // TODO: this should be an assertion
          Assert (retev!=nullptr);
          if (retev!=nullptr)
          {
            Trace("type_checker") << "...returns " << retev << std::endl;
            visited[cur] = retev;
            visit.pop_back();
            continue;
          }
          // if we failed running via compiled, revert for now
          Trace("type_checker") << "...returns null" << std::endl;
        }
        // otherwise, visit children
        visited[cur] = nullptr;
        if (ck==Kind::EVAL_IF_THEN_ELSE)
        {
          // special case: visit only the condition
          visit.push_back(children[0]);
        }
        else
        {
          visit.insert(visit.end(), children.begin(), children.end());
        }
        continue;
      }
      if (it->second.get() == nullptr)
      {
        std::vector<Expr> cchildren;
        for (Expr& cp : children)
        {
          it = visited.find(cp);
          if (it != visited.end())
          {
            cchildren.push_back(it->second);
          }
          else
          {
            cchildren.push_back(nullptr);
          }
        }
        evaluated = nullptr;
        bool newContext = false;
        bool canEvaluate = true;
        switch (ck)
        {
          case Kind::FAIL:
            // fail term means we immediately return
            return cur;
          case Kind::APPLY:
          {
            // if a program and all arguments are ground, run it
            Kind cck = cchildren[0]->getKind();
            if (cck==Kind::PROGRAM_CONST || cck==Kind::ORACLE)
            {
              // maybe already cached
              ExprTrie* et = &d_evalTrie;
              for (const Expr& e : cchildren)
              {
                et = &(et->d_children[e.get()]);
              }
              if (et->d_data!=nullptr)
              {
                evaluated = et->d_data;
              }
              else
              {
                Ctx newCtx;
                // see if we evaluate
                evaluated = evaluateProgramInternal(cchildren, newCtx);
                //std::cout << "Evaluate prog returned " << evaluated << std::endl;
                if (evaluated==nullptr || newCtx.empty())
                {
                  // if the evaluation can be shortcircuited, don't need to
                  // push a context
                  // store the base evaluation (if applicable)
                  et->d_data = evaluated;
                }
                else
                {
                  // otherwise push an evaluation scope
                  newContext = true;
                  ctxs.push_back(newCtx);
                  visits.emplace_back(std::vector<Expr>{evaluated});
                  visiteds.emplace_back();
                  ets.push_back(et);
                }
              }
            }
          }
            break;
          case Kind::EVAL_IF_THEN_ELSE:
          {
            Assert (cchildren[0]!=nullptr);
            // get the evaluation of the condition
            Literal * l = d_state.getLiteral(cchildren[0].get());
            if (l!=nullptr && l->d_tag==Literal::BOOL)
            {
              // inspect the relevant child only
              size_t index = l->d_bool ? 1 : 2;
              evaluated = cchildren[index];
              if (evaluated==nullptr)
              {
                canEvaluate = false;
                // evaluate the child if not yet done so
                visit.push_back(children[index]);
              }
            }
            else
            {
              // note we must evaluate the children so that e.g. beta-reduction
              // and more generally substitution is accurate for non-ground
              // terms.
              for (size_t i=1; i<3; i++)
              {
                if (cchildren[i]==nullptr)
                {
                  // evaluate the child if not yet done so
                  visit.push_back(children[i]);
                  // can't evaluate yet if we aren't finished evaluating
                  canEvaluate = false;
                }
              }
            }
          }
            break;
          default:
            if (isLiteralOp(ck))
            {
              evaluated = evaluateLiteralOpInternal(ck, cchildren);
            }
            break;
        }
        if (newContext)
        {
          break;
        }
        if (canEvaluate)
        {
          if (evaluated==nullptr)
          {
            evaluated = d_state.mkExprInternal(ck, cchildren);
          }
          visited[cur] = evaluated;
          visit.pop_back();
        }
      }
      else
      {
        visit.pop_back();
      }
    }
    // if we are done evaluating the current context
    if (visits.back().empty())
    {
      // get the result from the inner evaluation
      evaluated = visiteds.back()[init];
      // pop the evaluation context
      visiteds.pop_back();
      visits.pop_back();
      // set the result
      if (!visits.empty())
      {
        Trace("type_checker") << "EVALUATE " << init << ", " << ctxs.back()
                              << " = " << evaluated << std::endl;
        visiteds.back()[visits.back().back()] = evaluated;
        visits.back().pop_back();
        // store the evaluation
        Assert(!ets.empty());
        ets.back()->d_data = evaluated;
        ets.pop_back();
      }
      ctxs.pop_back();
    }
  }
  Trace("type_checker") << "EVALUATE " << e << ", " << ctx << " = " << evaluated
                        << std::endl;
  return evaluated;
}

Expr TypeChecker::evaluateProgram(const std::vector<Expr>& children, Ctx& newCtx)
{
  Expr ret = evaluateProgramInternal(children, newCtx);
  if (ret!=nullptr)
  {
    return ret;
  }
  // otherwise does not evaluate, return application
  return d_state.mkExprInternal(Kind::APPLY, children);
}

bool TypeChecker::isGround(const std::vector<Expr>& args)
{
  for (const Expr& e : args)
  {
    if (!e->isGround())
    {
      return false;
    }
  }
  return true;
}


int run(const std::string& call, std::ostream& response)
{
  FILE* stream = popen(call.c_str(), "r");
  if (stream != nullptr)
  {
    int ch;
    while ((ch = fgetc(stream)) != EOF)
    {
      response << (unsigned char)ch;
    }
    return pclose(stream);
  }
  return -1;
}

Expr TypeChecker::evaluateProgramInternal(const std::vector<Expr>& children,
                                          Ctx& newCtx)
{
  if (!isGround(children))
  {
    // do not evaluate on non-ground
    return nullptr;
  }
  const Expr& hd = children[0];
  Kind hk = hd->getKind();
  if (hk==Kind::PROGRAM_CONST)
  {
    if (hd->isCompiled())
    {
      Trace("type_checker") << "RUN program " << children << std::endl;
      Expr ret = run_evaluateProgram(children, newCtx);
      Trace("type_checker") << "...matches " << ret << ", ctx = " << newCtx << std::endl;
      return ret;
    }
    size_t nargs = children.size();
    std::map<Expr, Expr>::iterator it = d_programs.find(hd);
    Assert (it!=d_programs.end());
    if (it!=d_programs.end())
    {
      Trace("type_checker") << "INTERPRET program " << children << std::endl;
      // otherwise, evaluate
      std::vector<Expr>& progChildren = it->second->getChildren();
      for (Expr& c : progChildren)
      {
        newCtx.clear();
        Expr hd = c->getChildren()[0];
        std::vector<Expr>& hchildren = hd->d_children;
        if (nargs != hchildren.size())
        {
          // TODO: catch this during weak type checking of program bodies
          Warning() << "*** Bad number of arguments provided in function call to " << hd << std::endl;
          Warning() << "  Arguments: " << children << std::endl;
          return nullptr;
        }
        bool matchSuccess = true;
        for (size_t i=1; i<nargs; i++)
        {
          if (!match(hchildren[i], children[i], newCtx))
          {
            matchSuccess = false;
            break;
          }
        }
        if (matchSuccess)
        {
          Trace("type_checker")
              << "...matches " << hd << ", ctx = " << newCtx << std::endl;
          return c->getChildren()[1];
        }
      }
      Trace("type_checker") << "...failed to match." << std::endl;
    }
  }
  else if (hk==Kind::ORACLE)
  {
    // get the command
    std::string ocmd;
    if (!d_state.getOracleCmd(hd, ocmd))
    {
      return nullptr;
    }
    std::stringstream ss;
    for (size_t i=1, nchildren=children.size(); i<nchildren; i++)
    {
      ss << children[i] << std::endl;
    }
    Trace("oracles") << "Call oracle " << ocmd << " with arguments:" << std::endl;
    Trace("oracles") << "```" << std::endl;
    Trace("oracles") << ss.str() << std::endl;
    Trace("oracles") << "```" << std::endl;
    std::stringstream call;
    call << ocmd << " input.txt";
    std::stringstream response;
    int retVal = run(call.str(), response);
    if (retVal!=0)
    {
      Trace("oracles") << "...failed to run" << std::endl;
      return nullptr;
    }
    Trace("oracles") << "...got response \"" << response.str() << "\"" << std::endl;
    Parser poracle(d_state);
    poracle.setStringInput(response.str());
    Expr ret = poracle.parseNextExpr();
    Trace("oracles") << "returns " << ret << std::endl;
    return ret;
  }
  // just return nullptr, which should be interpreted as a failed evaluation
  return nullptr;
}

Expr TypeChecker::evaluateLiteralOp(Kind k, const std::vector<Expr>& args)
{
  Expr ret = evaluateLiteralOpInternal(k, args);
  if (ret!=nullptr)
  {
    return ret;
  }
  // otherwise does not evaluate, return application
  return d_state.mkExprInternal(k, args);
}

Expr getNAryChildren(Expr e, const Expr& op, std::vector<Expr>& children, bool isLeft, bool extractAll)
{
  while (e->getKind()==Kind::APPLY)
  {
    Expr cop = e->getChildren()[0];
    if (cop->getKind()!=Kind::APPLY)
    {
      break;
    }
    if (cop->getChildren()[0]!=op)
    {
      break;
    }
    // push back the element
    children.push_back(isLeft ? e->getChildren()[1] : cop->getChildren()[1]);
    // traverse to tail
    e = isLeft ? cop->getChildren()[1] : e->getChildren()[1];
    if (!extractAll && children.size()==2)
    {
      return e;
    }
  }
  // must be equal to the nil term
  return e;
}

  
Expr TypeChecker::evaluateLiteralOpInternal(Kind k, const std::vector<Expr>& args)
{
  Trace("type_checker") << "EVALUATE-LIT " << k << " " << args << std::endl;
  switch (k)
  {
    case Kind::EVAL_IS_EQ:
    {
      Assert (args.size()==2);
      // evaluation is indepdent of whether it is a literal
      bool ret = args[0]==args[1];
      if (ret)
      {
        return d_state.mkTrue();
      }
      else if (isGround(args))
      {
        return d_state.mkFalse();
      }
      return nullptr;
    }
    break;
    case Kind::EVAL_IF_THEN_ELSE:
    {
      // temporary
      Literal * l = d_state.getLiteral(args[0].get());
      if (l!=nullptr && l->d_tag==Literal::BOOL)
      {
        return args[l->d_bool ? 1 : 2];
      }
      /*
      // conditions equal
      if (args[1]==args[2])
      {
        return args[1];
      }
      */
      return nullptr;
    }
    break;
    case Kind::EVAL_REQUIRES:
    {
      if (args[0]==args[1])
      {
        return args[2];
      }
      Trace("type_checker")
        << "REQUIRES: failed " << args[0] << " == " << args[1] << std::endl;
      return nullptr;
    }
    case Kind::EVAL_CONS:
    case Kind::EVAL_APPEND:
    case Kind::EVAL_TO_LIST:
    case Kind::EVAL_FROM_LIST:
    {
      AppInfo* ac = d_state.getAppInfo(args[0].get());
      Assert (ac!=nullptr);
      Attr ck = ac->d_attrCons;
      Assert (ck==Attr::RIGHT_ASSOC_NIL || ck==Attr::LEFT_ASSOC_NIL);
      bool isLeft = (ck==Attr::LEFT_ASSOC_NIL);
      Trace("type_checker_debug") << "CONS: " << isLeft << " " << args << std::endl;
      Expr op = args[0];
      size_t tailIndex = (isLeft ? 1 : 2);
      size_t headIndex = (isLeft ? 2 : 1);
      // harg is either the head (cons/append) or the argument (to_list/from_list)
      const Expr& harg = args[args.size()==2 ? 1 : headIndex];
      if (!harg->isGround()) // or LIST?
      {
        // not ready
        Trace("type_checker_debug") << "...head is non-ground" <<std::endl;
        return nullptr;
      }
      Expr ret;
      std::vector<Expr> hargs;
      switch (k)
      {
        case Kind::EVAL_TO_LIST:
        {
          if (harg==ac->d_attrConsTerm)
          {
            // already nil
            return harg;
          }
          Expr a = harg;
          a = getNAryChildren(a, op, hargs, isLeft, false);
          if (!hargs.empty())
          {
            // already a list
            return harg;
          }
          // otherwise, turn into singleton list
          ret = ac->d_attrConsTerm;
          hargs.push_back(a);
        }
          break;
        case Kind::EVAL_FROM_LIST:
        {
          Expr a = harg;
          a = getNAryChildren(a, op, hargs, isLeft, false);
          if (hargs.size()==1)
          {
            if (a!=ac->d_attrConsTerm)
            {
              Warning() << "...failed to decompose " << harg << " in from_list" << std::endl;
              return nullptr;
            }
            // turn singleton list
            return hargs[0];
          }
          // otherwise self
          return harg;
        }
          break;
        case Kind::EVAL_CONS:
          ret = args[tailIndex];
          hargs.push_back(harg);
          break;
        case Kind::EVAL_APPEND:
        {
          ret = args[tailIndex];
          Expr a = harg;
          // Note we take the tail verbatim
          a = getNAryChildren(a, op, hargs, isLeft, true);
          if (a!=ac->d_attrConsTerm)
          {
            Warning() << "...failed to decompose " << harg << " in append" << std::endl;
            return nullptr;
          }
        }
          break;
        default:
          break;
      }
      std::vector<Expr> cc;
      cc.push_back(op);
      cc.push_back(nullptr);
      cc.push_back(nullptr);
      for (size_t i=0, nargs=hargs.size(); i<nargs; i++)
      {
        cc[tailIndex] = ret;
        cc[headIndex] = hargs[isLeft ? i : (nargs-1-i)];
        ret = d_state.mkApplyInternal(cc);
      }
      Trace("type_checker_debug") << "CONS: " << isLeft << " " << args << " -> " << ret << std::endl;
      return ret;
    }
    default:
      break;
  }
  if (!isGround(args))
  {
    Trace("type_checker") << "...does not evaluate (non-ground)" << std::endl;
    return nullptr;
  }
  // convert argument expressions to literals
  std::vector<Literal*> lits;
  for (const Expr& e : args)
  {
    Literal * l = d_state.getLiteral(e.get());
    // symbols are stored as literals but do not evaluate
    if (l==nullptr || l->d_tag==Literal::SYMBOL)
    {
      Trace("type_checker") << "...does not evaluate (argument)" << std::endl;
      // failed to convert an argument
      return nullptr;
    }
    lits.push_back(l);
  }
  // evaluate
  Literal eval = Literal::evaluate(k, lits);
  if (eval.d_tag==Literal::INVALID)
  {
    Trace("type_checker") << "...does not evaluate (return)" << std::endl;
    // failed to evaluate
    return nullptr;
  }
  // convert back to an expression
  Expr lit = d_state.mkLiteral(eval.toKind(), eval.toString());
  Trace("type_checker") << "...evaluates to " << lit << std::endl;
  return lit;
}

Expr TypeChecker::getLiteralOpType(Kind k,
                                   std::vector<Expr>& childTypes, 
                                   std::ostream* out)
{
  // NOTE: applications of most of these operators should only be in patterns,
  // where type checking is not strict.
  switch (k)
  {
    case Kind::EVAL_ADD:
    case Kind::EVAL_MUL:
    case Kind::EVAL_CONCAT:
    case Kind::EVAL_NEG:
      return childTypes[0];
    case Kind::EVAL_REQUIRES:
      return childTypes[2];
    case Kind::EVAL_IF_THEN_ELSE:
    case Kind::EVAL_CONS:
    case Kind::EVAL_APPEND:
    case Kind::EVAL_TO_LIST:
    case Kind::EVAL_FROM_LIST:
      return childTypes[1];
    case Kind::EVAL_IS_EQ:
    case Kind::EVAL_NOT:
    case Kind::EVAL_AND:
    case Kind::EVAL_OR:
    case Kind::EVAL_IS_NEG:
    case Kind::EVAL_IS_ZERO:
      return d_state.mkBoolType();
    case Kind::EVAL_INT_DIV:
    case Kind::EVAL_TO_INT:
    case Kind::EVAL_LENGTH:
      return getOrSetLiteralTypeRule(Kind::NUMERAL);
    case Kind::EVAL_RAT_DIV:
    case Kind::EVAL_TO_RAT:
      return getOrSetLiteralTypeRule(Kind::DECIMAL);
    default:break;
  }
  if (out)
  {
    (*out) << "Unknown type for literal operator " << k;
  }
  return nullptr;
}

}  // namespace alfc


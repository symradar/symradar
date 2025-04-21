package tools.safepatch.conditions;

import java.util.Map;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import com.microsoft.z3.ArithExpr;
import com.microsoft.z3.BoolExpr;
import com.microsoft.z3.Context;
import com.microsoft.z3.Expr;
import com.microsoft.z3.Solver;

public class BooleanDiff {

	private static final Logger logger = LogManager.getLogger();
	
	BoolExpr originalExpr;
	BoolExpr newExpr;

	private Context ctx;

	private Map<String, Expr> codeStringExprMap;
	private Map<String, String> typesMap = null;
	
	public BooleanDiff(BoolExpr originalExpr, BoolExpr newExpr, Context ctx, Map<String, Expr> codestringExprMap) {
		this.originalExpr = originalExpr;
		this.newExpr = newExpr;
		this.ctx = ctx;
		this.codeStringExprMap = codestringExprMap;
		if(logger.isDebugEnabled()) logger.debug("Original: {}", this.originalExpr);
		if(logger.isDebugEnabled()) logger.debug("New: {}", this.newExpr);
	}
	
	
	public void setTypesMap(Map<String, String> typesMap) {
		this.typesMap = typesMap;
	}


	/*
	 * Returns true if the new condition implies the original one
	 * for all the possible assignments.
	 * (new condition is subset of original condition).
	 * This is assumed to be an indicator that the condition becomes
	 * more restricted, since the two conditions are different, 
	 * because it means that the assignments that satisfy the new one
	 * are a subset of the ones that satisfy the old one.
	 * (Example: A && B => A, A && B is more restrictive than A) 
	 */
	public boolean newImpliesOriginal(){	
		return proveImplication(IMPL_DIRECTION.NEW_IMPLIES_ORIGINAL);
	}

	/*
	 * Returns true if the original condition implies the new one
	 * for all the possible assignments.
	 * (original condition is subset of new condition).
	 */
	public boolean originalImpliesNew(){	
		return proveImplication(IMPL_DIRECTION.ORIGINAL_IMPLIES_NEW);
	}
	
	/*
	 * True when "New <=> original" for all the possible assignments, 
	 * at a boolean level the two conditions are the same.
	 */
	public boolean biImplication(){
		return newImpliesOriginal() && originalImpliesNew();
	}
	
	/*
	 * True when "original AND new" is unsatisfiable, there exists no solutions
	 * that satisfies both the conditions together. 
	 */
	public boolean disjunction(){
		if(logger.isDebugEnabled()) logger.debug("Proving disjunction.");
		Solver solver = ctx.mkSolver();
		this.assertUnsigned(solver);
		
		solver.add(ctx.mkAnd(this.originalExpr, this.newExpr));
		return checkUnsat(solver);
	}
	
	/*
	 * True when "original AND new" is satisfiable (so there exists some solution
	 * that satisfies both) but none of the two is a strict subset of the other.
	 * This means that "new => old" is sat but not valid, and the same for
	 * "old => new".
	 */
	public boolean intersection(){
		if(logger.isDebugEnabled()) logger.debug("Proving intersection.");
		Solver solver = ctx.mkSolver();
		this.assertUnsigned(solver);
		
		solver.add(ctx.mkAnd(this.originalExpr, this.newExpr));
		return checkSat(solver) && !newImpliesOriginal() && !originalImpliesNew();
	}
	
	private enum IMPL_DIRECTION{
		NEW_IMPLIES_ORIGINAL,
		ORIGINAL_IMPLIES_NEW
	}
	
	
	private boolean proveImplication(IMPL_DIRECTION dir){
		Solver solver = ctx.mkSolver();
		this.assertUnsigned(solver);
		
		if(dir == IMPL_DIRECTION.NEW_IMPLIES_ORIGINAL) if(logger.isDebugEnabled()) logger.debug("Proving that new implies original.");
		else if(logger.isDebugEnabled()) logger.debug("Proving that original implies new.");
		
		//Prove that new => old by proving that (old =>)
		BoolExpr implication = (dir == IMPL_DIRECTION.NEW_IMPLIES_ORIGINAL ? 
				ctx.mkImplies(this.newExpr, this.originalExpr) :
					ctx.mkImplies(this.originalExpr, this.newExpr));
		
		BoolExpr notImpl = ctx.mkNot(implication);
		solver.add(notImpl);
		if(logger.isDebugEnabled()) logger.debug("Proving implication: {}", implication);
		if(logger.isDebugEnabled()) logger.debug("To prove we want {} to be unsat.", implication);
		
		return checkUnsat(solver);
	}
	
	private boolean checkUnsat(Solver solver){
		
		com.microsoft.z3.Status status = solver.check();
		if(logger.isDebugEnabled()) logger.debug("Status: {}", status);
		
		if(status == com.microsoft.z3.Status.UNSATISFIABLE) return true;
		else if(status == com.microsoft.z3.Status.SATISFIABLE){
			if(logger.isDebugEnabled()) logger.debug("Model: {}", solver.getModel());
			return false;
		} else return false;
	}
	
	private boolean checkSat(Solver solver){	
		com.microsoft.z3.Status status = solver.check();
		if(logger.isDebugEnabled()) logger.debug("Status: {}", status);
		
		if(status == com.microsoft.z3.Status.UNSATISFIABLE) return false;
		else if(status == com.microsoft.z3.Status.SATISFIABLE){
			if(logger.isDebugEnabled()) logger.debug("Model: {}", solver.getModel());
			return true;
		} else return false;
	}

	private void assertUnsigned(Solver solver){
		if(this.typesMap != null){
			for(String var : this.codeStringExprMap.keySet()){
				if(this.typesMap.get(var) != null &&
						this.typesMap.get(var).startsWith("u")){
					if(logger.isDebugEnabled()) logger.debug("Asserting unsigned: {}", var); 
					solver.add(ctx.mkOr(
							ctx.mkGt((ArithExpr) this.codeStringExprMap.get(var), ctx.mkInt(0)),
							ctx.mkEq((ArithExpr) this.codeStringExprMap.get(var), ctx.mkInt(0))));
				}
			}
		}
	}
	
}

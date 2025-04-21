package tools.safepatch.tests;

import com.microsoft.z3.BoolExpr;
import com.microsoft.z3.Context;
import com.microsoft.z3.Goal;
import com.microsoft.z3.Solver;
import com.microsoft.z3.Tactic;
import com.sun.org.apache.xerces.internal.util.Status;


/*
 * - Try to use z3 to prove that new => old:
 * 		
	(declare-const A Bool)
	(declare-const B Bool)
	(declare-const C Bool)
	(declare-const D Bool)
	(declare-const E Bool)
	(declare-const F Bool)
	(declare-const G Bool)
	(define-fun old () Bool
	  (or (and (or (and A B) (not C)) (or D F)) (not A))
	)
	(define-fun new () Bool
	  (and (or (and (or (and A B) (not C)) (or D F)) (not A)) (or (not (or D (and F (not G)))) (and G (not A))))
	)
	(define-fun implication () Bool
	  (=> new old)
	)
	
	;FIRST WAY: PROVE THAT THE IMPLICATION IS VALID
	;which means that the negation of the implication is unsat
	(push)
	(assert (not implication))
	(check-sat)
	(pop)
	
	;SECOND WAY: SIMPLIFY THE IMPLICATION
	;If it is valid it should simplify to TRUE
	;which means that the output list of goals will be empty
	(push)
	(assert implication)
	(apply ctx-solver-simplify)
	(pop)
		
 * 
 */

public class PlayingWithZ3 {

	public static void main(String[] args) {
		System.out.println("Creating Z3 Context...");
		Context ctx = new Context();
		System.out.println("Context created.");
        /* do something with the context */
		
		Solver s = ctx.mkSolver();
		
		//Like this I define Bool symbols
		//I could put them in a Map<String, BoolExpr>
		BoolExpr a = ctx.mkBoolConst("A");
		System.out.println(a);
		BoolExpr b =  ctx.mkBoolConst("B");
		System.out.println(b);
		
		BoolExpr oldCond = a;
		System.out.println(oldCond);
		BoolExpr newCond = ctx.mkAnd(a, b);
		System.out.println(newCond);
		
		BoolExpr implication = ctx.mkImplies(newCond, oldCond);
		System.out.println(implication);
		
		//Checking that the negation of the implication is unsatisfiable (so that the implication is valid)
		s.add(ctx.mkNot(implication));
		System.out.println(s);
		
		System.out.println("CHECKING: ");
		com.microsoft.z3.Status status = s.check();
		System.out.println(status);
		
		System.out.println();
		System.out.println("SIMPLIFYING: ");
//		for(String tactic : ctx.getTacticNames())
//			if(tactic.contains("simplify"))
//				System.out.println(tactic);
		Tactic t = ctx.mkTactic("ctx-solver-simplify");
		//Don't know how to use it...
		
        /* be kind to dispose manually and not wait for the GC. */
		System.out.println("Closing context...");
		ctx.close();
		System.out.println("Context closed.");
	}
}


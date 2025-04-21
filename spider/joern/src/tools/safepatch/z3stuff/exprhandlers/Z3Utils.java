/**
 * 
 */
package tools.safepatch.z3stuff.exprhandlers;

import java.util.Random;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import com.microsoft.z3.BitVecExpr;
import com.microsoft.z3.BoolExpr;
import com.microsoft.z3.Context;
import com.microsoft.z3.Expr;
import com.microsoft.z3.Solver;

/**
 * @author machiry
 *
 */
public class Z3Utils {
	
	private static final int BV_SIZE = 32;
	private static final Logger logger = LogManager.getLogger();
	
	private static String getRandomStr() {
        String SALTCHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        StringBuilder salt = new StringBuilder();
        Random rnd = new Random();
        while (salt.length() < 18) { // length of the random string.
            int index = (int) (rnd.nextFloat() * SALTCHARS.length());
            salt.append(SALTCHARS.charAt(index));
        }
        String saltStr = salt.toString();
        return saltStr;

    }
	
	public static Object getTop(Context ctx) {
		Object retVal = (BitVecExpr)ctx.mkBVConst(getRandomStr(), 64);
		return retVal;
	}
	
	public static Expr convertBoolToBitVec(Context ctx, BoolExpr newBr) {
		return ctx.mkITE(newBr, (Expr)generateConstant(ctx, 1), (Expr)generateConstant(ctx, 0));
	}
	
	public static boolean checkImplication(Context ctx, Object oldCond, Object newCond) {
		/*BoolExpr targetCond = ctx.mkImplies((BoolExpr)oldCond, (BoolExpr)newCond);
		targetCond = ctx.mkNot(targetCond);*/
		/*Solver solver = ctx.mkSolver();
		solver.add(targetCond);
		com.microsoft.z3.Status status = solver.check();
		boolean retval = (status == com.microsoft.z3.Status.UNSATISFIABLE);*/
		Solver solver2 = ctx.mkSolver();
		solver2.add((BoolExpr)oldCond);
		solver2.add(ctx.mkNot((BoolExpr)newCond));
		com.microsoft.z3.Status status = solver2.check();
		boolean retval = status == com.microsoft.z3.Status.UNSATISFIABLE;
		if(!retval) {
			logger.debug("Condition Implication failed, Old condition:\n{}\nNew condition:\n{}", oldCond.toString(), newCond.toString());
		}
		return retval;
		
	}
	
	public static boolean checkEquivalence(Context ctx, Object oldCond, Object newCond) {
		Solver solver2 = ctx.mkSolver();
		if((oldCond instanceof BitVecExpr && newCond instanceof BoolExpr) || 
			(oldCond instanceof BoolExpr && newCond instanceof BitVecExpr)) {
			if(oldCond instanceof BoolExpr) {
				oldCond = convertBoolToBitVec(ctx, (BoolExpr)oldCond);
			}
			if(newCond instanceof BoolExpr) {
				newCond = convertBoolToBitVec(ctx, (BoolExpr)newCond);
			}
		}
		solver2.add(ctx.mkNot(ctx.mkEq((Expr)oldCond, (Expr)newCond)));
		com.microsoft.z3.Status status = solver2.check();
		boolean retval = status == com.microsoft.z3.Status.UNSATISFIABLE;
		if(!retval) {
			logger.debug("Expression equivalence failed, Old expression:\n{}\nNew expression:\n{}", oldCond.toString(), newCond.toString());
		}
		return retval;
	}
	
	public static boolean checkSatisfiable(Context ctx, Object oldCond, Object newCond) {
		BoolExpr targetCond = ctx.mkImplies((BoolExpr)oldCond, (BoolExpr)newCond);
		Solver solver = ctx.mkSolver();
		solver.add(targetCond);
		com.microsoft.z3.Status status = solver.check();
		return (status == com.microsoft.z3.Status.SATISFIABLE);
		
	}
	
	private static Object convertBittoBinOp(Context ctx, Object targetObj, String op) {
		Object retVal = targetObj;
		switch(op) {
		case "&&":
			if(targetObj instanceof BitVecExpr) {
				retVal = ctx.mkNot(ctx.mkEq((Expr)targetObj, (Expr)generateConstant(ctx, 0)));
			}
			break;
		case "||":
			if(targetObj instanceof BitVecExpr) {
				retVal = ctx.mkNot(ctx.mkEq((Expr)targetObj, (Expr)generateConstant(ctx, 0)));
			}
			break;
		}
		return retVal;
	}
	
	private static Object convertBintoBitVec(Context ctx, Object targetObj, String op) {
		Object retVal = targetObj;
		if(targetObj instanceof BoolExpr) {
			switch(op) {
				case "+":
				case "-":
				case "*":
				case "%":
				case "&":
				case "|":
				case "^":
				case "<<":
				case ">>":
					retVal = convertBoolToBitVec(ctx, (BoolExpr)targetObj);
					break;
			}
		}		
		return retVal;
	}
	
	public static Object processBinOp(Context ctx, Object leftOp, Object rightOp, String op) {
		/*if(!op.equals("==") && !op.equals("!=")) {
			if(!(leftOp instanceof BitVecExpr)) {
				logger.debug("Generating top for left operand");
				leftOp = Z3Utils.getTop(ctx);
			}
			if(!(rightOp instanceof BitVecExpr)) {
				logger.debug("Generating top for right operand");
				rightOp = Z3Utils.getTop(ctx);
			}
		}*/
		leftOp = convertBittoBinOp(ctx, leftOp, op);
		leftOp = convertBintoBitVec(ctx, leftOp, op);
		rightOp = convertBittoBinOp(ctx, rightOp, op);
		rightOp = convertBintoBitVec(ctx, rightOp, op);
		switch (op) {
			case "+":
				//return ctx.mkAdd((ArithExpr)leftOp, (ArithExpr)rightOp);
				return ctx.mkBVAdd((BitVecExpr)leftOp, (BitVecExpr)rightOp);
			case "-":
				//return ctx.mkSub((ArithExpr)leftOp, (ArithExpr)rightOp);
				return ctx.mkBVSub((BitVecExpr)leftOp, (BitVecExpr)rightOp);
			case "*":
				//return ctx.mkMul((ArithExpr)leftOp, (ArithExpr)rightOp);
				return ctx.mkBVMul((BitVecExpr)leftOp, (BitVecExpr)rightOp);
			case "/":
				//return ctx.mkDiv((ArithExpr)leftOp, (ArithExpr)rightOp);
				return ctx.mkBVUDiv((BitVecExpr)leftOp, (BitVecExpr)rightOp);
			case "%":
				//return ctx.mkAdd((IntExpr)leftOp, (IntExpr)rightOp);
				return ctx.mkBVURem((BitVecExpr)leftOp, (BitVecExpr)rightOp);
			case "&":			
				/*return  ctx.mkBV2Int(ctx.mkBVAND(ctx.mkInt2BV(BV_SIZE, 
					     	     			 (IntExpr)leftOp),
					     	     			 ctx.mkInt2BV(BV_SIZE, 
					     	     			 (IntExpr)rightOp)), true);*/
				
				return ctx.mkBVAND((BitVecExpr)leftOp, (BitVecExpr)rightOp);
			case "&&":
				return ctx.mkAnd((BoolExpr)leftOp, (BoolExpr)rightOp);
			case "|":
				/*return  ctx.mkBV2Int(ctx.mkBVOR(ctx.mkInt2BV(BV_SIZE, 
	     			 						(IntExpr)leftOp),
	     			 						ctx.mkInt2BV(BV_SIZE, 
	     			 						(IntExpr)rightOp)), true);*/
				return ctx.mkBVOR((BitVecExpr)leftOp, (BitVecExpr)rightOp);
			case "||":
				return ctx.mkOr((BoolExpr)leftOp, (BoolExpr)rightOp);
			case "^":
				/*return  ctx.mkBV2Int(ctx.mkBVXOR(ctx.mkInt2BV(BV_SIZE, 
	     			 						(IntExpr)leftOp),
	     			 						ctx.mkInt2BV(BV_SIZE, 
	     			 						(IntExpr)rightOp)), true);*/
				return ctx.mkBVXOR((BitVecExpr)leftOp, (BitVecExpr)rightOp);
			case "<<":
				/*return  ctx.mkBV2Int(ctx.mkBVSHL(ctx.mkInt2BV(BV_SIZE, 
											(IntExpr)leftOp),
											ctx.mkInt2BV(BV_SIZE, 
											(IntExpr)rightOp)), true);*/
				return  ctx.mkBVSHL((BitVecExpr)leftOp,(BitVecExpr)rightOp);
			case ">>":
				/*return  ctx.mkBV2Int(ctx.mkBVASHR(ctx.mkInt2BV(BV_SIZE, 
	     			 						(IntExpr)leftOp),
	     			 						ctx.mkInt2BV(BV_SIZE, 
	     			 						(IntExpr)rightOp)), true);*/
				return  ctx.mkBVASHR((BitVecExpr)leftOp,(BitVecExpr)rightOp);
			case ">":
				//return ctx.mkGt((ArithExpr)leftOp, (ArithExpr)rightOp);
				return ctx.mkBVSGT((BitVecExpr)leftOp, (BitVecExpr)rightOp);

			case "<":
				//return ctx.mkLt((ArithExpr)leftOp, (ArithExpr)rightOp);
				return ctx.mkBVSLT((BitVecExpr)leftOp, (BitVecExpr)rightOp);

			case ">=":
				if(logger.isDebugEnabled()) logger.debug("Making >=");
				/*return ctx.mkOr(
						ctx.mkGt((ArithExpr)leftOp, (ArithExpr)rightOp),
						ctx.mkEq((ArithExpr)leftOp, (ArithExpr)rightOp));*/
				return ctx.mkBVSGE((BitVecExpr)leftOp, (BitVecExpr)rightOp);

			case "<=":
				if(logger.isDebugEnabled()) logger.debug("Making <=");
				/*return ctx.mkOr(
						ctx.mkLt((ArithExpr)leftOp, (ArithExpr)rightOp),
						ctx.mkEq((ArithExpr)leftOp, (ArithExpr)rightOp));*/
				return ctx.mkBVSLE((BitVecExpr)leftOp, (BitVecExpr)rightOp);

			case "==":
				//return ctx.mkEq((ArithExpr)leftOp, (ArithExpr)rightOp);
				//return ctx.mkBV(ctx.mkBVSGT((BitVecExpr)leftOp, (BitVecExpr)rightOp), ctx.mkBVSGT((BitVecExpr)leftOp, (BitVecExpr)rightOp));
				return ctx.mkEq((Expr)leftOp, (Expr)rightOp);
			case "!=":
				return ctx.mkNot(ctx.mkEq((Expr)leftOp, (Expr)rightOp));
			default:
				logger.error("Unknown BinaryOperator");
				break;

		}
		return null;
	}
	
	public static Object generateConstant(Context ctx, long value) {
		return ctx.mkBV(value, 64);
	}
	
	public static Object processIncrement(Context ctx, Object oper) {
		Object rightOp = Z3Utils.generateConstant(ctx, 1);
		return Z3Utils.processBinOp(ctx, oper, rightOp, "+");
	}
	
	public static Object processDecrement(Context ctx, Object oper) {
		Object rightOp = Z3Utils.generateConstant(ctx, 1);
		return Z3Utils.processBinOp(ctx, oper, rightOp, "-");
	}
	
	public static Object processUnaryOp(Context ctx, Object oper, String op) {
		switch(op) {
			case "+":
				return oper;
			case "-":
				// pretty fancy: 2's complement :)
				Object oneConst = Z3Utils.generateConstant(ctx, 1);
				Object leftObj = ctx.mkBVNeg((BitVecExpr)oper);
				return Z3Utils.processBinOp(ctx, leftObj, oneConst, "+");
			case "--":
				return processDecrement(ctx, oper);
			case "++":
				return processIncrement(ctx, oper);
			case "~":
				return ctx.mkBVNeg((BitVecExpr)oper);
			case "&":
			case "*":
				return oper;
			case "!":
				if(oper instanceof BoolExpr) {
					return ctx.mkNot((BoolExpr)oper);
				} else {
					return ctx.mkNot(ctx.mkEq((Expr)oper, (Expr)Z3Utils.generateConstant(ctx, 0)));
				}
			default:
				logger.error("Unknown unary op: {}", op);
		}
		return null;
		
	}
}

package tools.safepatch.util;

import ast.statements.CompoundStatement;
import ast.statements.Condition;
import ast.statements.DoStatement;
import ast.statements.ForStatement;
import ast.statements.IfStatement;
import ast.statements.SwitchStatement;
import ast.statements.WhileStatement;
import cfg.nodes.ASTNodeContainer;

public class CFGUtil {

	public static boolean isIfStart(ASTNodeContainer n){
		return n.getASTNode() instanceof Condition && n.getASTNode().getParent() instanceof IfStatement;
	}
	
	public static boolean isWhileStart(ASTNodeContainer n){
		return n.getASTNode() instanceof Condition && n.getASTNode().getParent() instanceof WhileStatement;
	}
	
	public static boolean isForStart(ASTNodeContainer n){
		return n.getASTNode() instanceof Condition && n.getASTNode().getParent() instanceof ForStatement;
	}
	
	public static boolean isSwitchStart(ASTNodeContainer n){
		return n.getASTNode() instanceof Condition && n.getASTNode().getParent() instanceof SwitchStatement;
	}
	
	public static boolean isDoStart(ASTNodeContainer n){
		return (n.getASTNode().getParent() instanceof CompoundStatement && n
				.getASTNode().getParent().getParent() instanceof DoStatement)
				|| n.getASTNode().getParent() instanceof DoStatement;
	}
	
}

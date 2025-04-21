/**
 * 
 */
package tools.safepatch;

import java.util.ArrayList;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ast.declarations.ClassDefStatement;
import ast.functionDef.FunctionDef;
import ast.statements.IdentifierDeclStatement;
import ast.walking.ASTNodeVisitor;

/**
 * @author machiry
 * @author Eric Camellini
 * 
 */
public class SafepatchASTNodeVisitor extends ASTNodeVisitor{
	
	ArrayList<FunctionDef> functionList = null;

	public ArrayList<FunctionDef> getFunctionList() {
		return functionList;
	}

	public void setFunctionList(ArrayList<FunctionDef> functionList) {
		this.functionList = functionList;
	}

	public void visit(FunctionDef node)
	{
		if (functionList != null)
			functionList.add(node);
	}

	public void visit(ClassDefStatement node)
	{
	}

	public void visit(IdentifierDeclStatement node)
	{
	}

}

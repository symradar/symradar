/**
 * 
 */
package tools.safepatch;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Stack;

import org.antlr.v4.runtime.ParserRuleContext;
import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ast.ASTNode;
import ast.ASTNodeBuilder;
import ast.functionDef.FunctionDef;
import ast.walking.ASTNodeVisitor;
import ast.walking.ASTWalker;

/**
 * @author machiry
 * @author Eric Camellini
 * 
 */
public class SafepatchASTWalker extends ASTWalker {
	
	private static final Logger logger = LogManager.getLogger("SafepatchASTWalker");
	
	ASTNodeVisitor currVisitor = new SafepatchASTNodeVisitor();
	HashMap<String, ArrayList<FunctionDef>> fileFunctionList;

	public HashMap<String, ArrayList<FunctionDef>> getFileFunctionList() {
		return fileFunctionList;
	}

	public SafepatchASTWalker() {
		org.apache.logging.log4j.core.config.Configurator.setLevel("SafepatchASTWalker", Level.OFF);
		fileFunctionList = new HashMap<>();
	}
	
	@Override
	public void startOfUnit(ParserRuleContext ctx, String filename) {

		if(logger.isDebugEnabled()) logger.debug("startOfUnit:" + filename);
		((SafepatchASTNodeVisitor) currVisitor).setFunctionList(new ArrayList<FunctionDef>());
	}

	@Override
	public void endOfUnit(ParserRuleContext ctx, String filename) {
	
		if(logger.isDebugEnabled()) logger.debug("endOfUnit:" + filename);
		ArrayList<FunctionDef> functionList = ((SafepatchASTNodeVisitor) currVisitor).getFunctionList();
		if(logger.isDebugEnabled()) logger.debug("Functions found in the file:");
		for(FunctionDef f: functionList)
			if(logger.isDebugEnabled()) logger.debug(f.name.getEscapedCodeStr());
		fileFunctionList.put(filename, functionList);
	}

	@Override
	public void processItem(ASTNode node, Stack<ASTNodeBuilder> nodeStack) {
		if(logger.isDebugEnabled()) logger.debug("processItem:" + node + " " + node.getLocation());

		node.accept(currVisitor);
	}

}

package udg.useDefAnalysis.environments;

import java.util.LinkedList;

import udg.ASTNodeASTProvider;
import udg.ASTProvider;
import udg.symbols.CodeStringSymbol;
import udg.symbols.UseOrDefSymbol;

public class UnaryOpEnvironment extends EmitUseEnvironment
{
	
	public void addChildSymbols(LinkedList<UseOrDefSymbol> childSymbols,
			ASTProvider child)
	{
		
		String codeStr = astProvider.getEscapedCodeStr();
		
		if(codeStr != null && codeStr.startsWith("&")){
			for(UseOrDefSymbol symbol : childSymbols){
				symbols.add(symbol.sorroundWithUnaryOperator(
						((ASTNodeASTProvider) astProvider).getASTNode(), 
						"&"));
			}
			return;
		}
		
		if(codeStr == null|| !codeStr.startsWith("*")){
			symbols.addAll(childSymbols);
			return;
		}
			
		LinkedList<UseOrDefSymbol> retval = new LinkedList<UseOrDefSymbol>();
			
		// emit all symbols as '* symbol'
		
		LinkedList<UseOrDefSymbol> derefedChildren = new LinkedList<UseOrDefSymbol>();
		for(UseOrDefSymbol c : childSymbols){
			derefedChildren.add(c.sorroundWithUnaryOperator(
					((ASTNodeASTProvider) astProvider).getASTNode(), 
					"*"));
		}
		
		retval.addAll(derefedChildren);

		// emit entire code string
		CodeStringSymbol c = new CodeStringSymbol(codeStr, 
				((ASTNodeASTProvider) astProvider).getASTNode());
		c.addChildSymbols(childSymbols);
		retval.add(c);
	
		useSymbols.addAll(childSymbols);
		symbols.addAll(retval);
		
	}
}

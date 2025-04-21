package udg.useDefAnalysis.environments;

import java.util.LinkedList;

import udg.ASTNodeASTProvider;
import udg.ASTProvider;
import udg.symbols.UseOrDefSymbol;

public class ArrayIndexingEnvironment extends EmitUseEnvironment
{

	public void addChildSymbols(LinkedList<UseOrDefSymbol> childSymbols,
			ASTProvider child)
	{
		
		LinkedList<UseOrDefSymbol> derefedChildren = new LinkedList<UseOrDefSymbol>();
		for(UseOrDefSymbol c : childSymbols){
			derefedChildren.add(c.sorroundWithUnaryOperator(
					((ASTNodeASTProvider) astProvider).getASTNode(), 
					"*"));
		}
			
		symbols.addAll(derefedChildren);
			
		useSymbols.addAll(childSymbols);
	}

}

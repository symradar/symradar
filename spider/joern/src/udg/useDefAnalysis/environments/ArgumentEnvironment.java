package udg.useDefAnalysis.environments;

import java.util.LinkedList;

import udg.ASTNodeASTProvider;
import udg.ASTProvider;
import udg.symbols.UnaryOperatorSymbol;
import udg.symbols.UseOrDefSymbol;

public class ArgumentEnvironment extends EmitDefAndUseEnvironment
{

	boolean isTainted = false;

	public void addChildSymbols(LinkedList<UseOrDefSymbol> childSymbols,
			ASTProvider child)
	{
		if (isDef(child)){
			// For tainted arguments, add "* symbol" instead of symbol
			// to defined symbols. Make an exception if symbol starts with '& '
			
			LinkedList<UseOrDefSymbol> derefChildSymbols = new LinkedList<UseOrDefSymbol>();
			for(UseOrDefSymbol symbol : childSymbols){
				
				if(!(symbol instanceof UnaryOperatorSymbol)){
					derefChildSymbols.add(symbol.sorroundWithUnaryOperator(
							((ASTNodeASTProvider) astProvider).getASTNode(), 
							"*"));
				} else {
					UnaryOperatorSymbol s = (UnaryOperatorSymbol) symbol;
					if(!s.getOperatorValue().equals("&")){
						derefChildSymbols.add(s.sorroundWithUnaryOperator(
								((ASTNodeASTProvider) astProvider).getASTNode(), 
								"*"));
						// !patch to see if we can detect macro-sources!
						derefChildSymbols.add(s);
					}else
						derefChildSymbols.add(s.getNestedUseOrDefSymbol());
				}
			}
			
			defSymbols.addAll(derefChildSymbols);
		}
		if (isUse(child))
			useSymbols.addAll(childSymbols);
	}
	
	public boolean isUse(ASTProvider child)
	{
		return true;
	}

	public boolean isDef(ASTProvider child)
	{
		return isTainted;
	}

	public void setIsTainted()
	{
		isTainted = true;
	}

}

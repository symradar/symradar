package udg.useDefAnalysis.environments;

import java.util.LinkedList;

import ast.ASTNode;
import udg.ASTNodeASTProvider;
import udg.ASTProvider;
import udg.symbols.UseOrDefSymbol;

public class AssignmentEnvironment extends EmitDefEnvironment
{

	@Override
	public boolean isUse(ASTProvider child)
	{
		int childNum = child.getChildNumber();

		if (childNum == 0)
		{

			String operatorCode = astProvider.getOperatorCode();
			if (operatorCode != null && !operatorCode.equals("="))
				return true;
			else
				return false;
		}

		return true;
	}
	
	@Override
	public void addChildSymbols(LinkedList<UseOrDefSymbol> childSymbols,
			ASTProvider child)
	{
		if (isDef(child))
		{
			boolean handled = false;
			if(childSymbols.size() > 1) {
				//OK, We cannot define more than one value.
				// lets check
				if(child instanceof ASTNodeASTProvider) {
					ASTNodeASTProvider targetChild = (ASTNodeASTProvider)child;
					ASTNode targetNode = targetChild.getASTNode();
					if(targetNode.getTypeAsString().equals("MemberAccess")) {
						String realDef = targetNode.getEscapedCodeStr().replaceAll("\\s+","");
						for(UseOrDefSymbol currSym:childSymbols) {
							String currSymStr = currSym.getNode().getEscapedCodeStr().replaceAll("\\s+", "");
							if(currSymStr.equals(realDef)) {
								defSymbols.add(currSym);
							} else {
								symbols.add(currSym);
							}
						}
					}
					handled = true;
				}
			}
			if(!handled) {
				defSymbols.addAll(childSymbols);
			}
		}
		if (isUse(child))
		{
			symbols.addAll(childSymbols);
		}
	}

	@Override
	public boolean isDef(ASTProvider child)
	{
		int childNum = child.getChildNumber();

		if (childNum == 0)
			return true;

		return false;
	}

}

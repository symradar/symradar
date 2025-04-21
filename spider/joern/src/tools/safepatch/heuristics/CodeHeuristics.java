package tools.safepatch.heuristics;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ast.ASTNode;
import ast.declarations.IdentifierDecl;
import ast.expressions.AdditiveExpression;
import ast.expressions.Argument;
import ast.expressions.ArgumentList;
import ast.expressions.AssignmentExpr;
import ast.expressions.CallExpression;
import ast.expressions.Expression;
import ast.expressions.Identifier;
import ast.expressions.PrimaryExpression;
import ast.expressions.UnaryOp;
import ast.statements.BreakStatement;
import ast.statements.CompoundStatement;
import ast.statements.ContinueStatement;
import ast.statements.ExpressionStatement;
import ast.statements.GotoStatement;
import ast.statements.IdentifierDeclStatement;
import ast.statements.Label;
import ast.statements.ReturnStatement;
import ast.statements.Statement;
import tools.safepatch.fgdiff.ASTDiff;
import tools.safepatch.fgdiff.ASTDiff.ACTION_TYPE;
import tools.safepatch.flowres.FlowRestrictiveMainConfig;

/**
 * @author Eric Camellini
 *
 */
public class CodeHeuristics {
	
	private static final Logger logger = LogManager.getLogger("CodeHeu");
	
	private int returnErrorScore = 1;
	private int stmtListErrorScore = 2;
	
	public CodeHeuristics() {
		org.apache.logging.log4j.core.config.Configurator.setLevel("CodeHeu", Level.OFF);
	}
	
	public int getReturnErrorScore() {
		return returnErrorScore;
	}

	public void setReturnErrorScore(int returnErrorScore) {
		this.returnErrorScore = returnErrorScore;
	}

	
	public int getCompoundErrorScore() {
		return stmtListErrorScore;
	}

	public void setCompoundErrorScore(int compoundErrorScore) {
		this.stmtListErrorScore = compoundErrorScore;
	}

	/**
	 * Returns the score of the r ReturnStatement after applying the heuristics.
	 */
	public int errorHeuristicsScore(ReturnStatement r) {

		int score = 0;
		ASTNode returnValue = r.getChild(0);
		if(returnValue == null) score++;
		else {
			// Return value is prepended by -
			if (returnValue instanceof UnaryOp && returnValue.getChild(0)
					.getEscapedCodeStr().equals(MINUS_SIGN))
				score++;

			// The return value code string contains one of the constants in errorCodes
			if (errorCodes.stream().anyMatch(errorCode -> returnValue
					.getEscapedCodeStr().contains(errorCode)))
				score++;

			// WEAK: The return value code string contains the keyword "err" or "null"
			if(returnValue.getEscapedCodeStr().toLowerCase().contains(ERROR_KEYWORD) ||
					returnValue.getEscapedCodeStr().toLowerCase().equals(NULL) ||
					returnValue.getEscapedCodeStr().toLowerCase().equals(FALSE) ||
					errorKeywords.stream().anyMatch(kw -> returnValue.getEscapedCodeStr().toLowerCase().contains(kw))) 
				score++;
		}
		//TODO more heuristics and score adjustments ?
		
		return score;
	}

	/**
	 * Returns true if the r ReturnStatement is classified as an error return statement,
	 * i.e. if the errorHeuristicScore method returns a value greater than or equal to
	 * the value of the returnErrorScore attribute.
	 */
	public boolean isError(ReturnStatement r){
		return errorHeuristicsScore(r) >= returnErrorScore;
	}
	
	
	/**
	 * Returns true if the statements list is classified as an error block of code,
	 * i.e. if the errorHeuristicScore method returns a value greater than or equal to
	 * the value of the compoundErrorScore attribute.
	 */
	public boolean isError(List<ASTNode> statements){
		return errorHeuristicScore(statements) >= stmtListErrorScore;
	}
	
	
	/**
	 * Returns the score of the statements list after applying all the
	 * heuristic (see doc. of the code of the function).
	 */
	public int errorHeuristicScore(List<ASTNode> statements){

		//if(logger.isDebugEnabled()) logger.debug("Checking error score for {}", statements);
		
		int score = 0;
		if(statements.isEmpty() ||
				statements.stream().anyMatch(stmt -> !(stmt instanceof Statement)))
			return score;
		
		// it ends with an error ReturnStatement, according to the isError method;
		if (statements.get(statements.size() - 1) instanceof ReturnStatement
				&& isError((ReturnStatement) statements
						.get(statements.size() - 1))){
			//if(logger.isDebugEnabled()) logger.debug("Ends with error return statement");
			score += 2;
		}
		
		// it has only one statement and it has return 0
		if(statements.size() == 1 && 
		   statements.get(0) instanceof ReturnStatement) { 
			ReturnStatement retSt = (ReturnStatement)statements.get(0);
			if(retSt.getChildren() != null && !retSt.getChildren().isEmpty()) {
				String currst = retSt.getChild(0).getEscapedCodeStr();
				if(currst.equals("0")) {
					score += 2;
				}
			}
		}
		
		// One of the statements in the basic block contains one of the error codes and it's not a return
		if(statements.stream().anyMatch(stmt -> !(stmt instanceof ReturnStatement) && errorCodes.stream().anyMatch(e -> stmt.getEscapedCodeStr().contains(e)))){
			if(logger.isDebugEnabled()) logger.debug("Contains an error code in one of the non return statements");
			score += 2;
		}
		if(statements.get(statements.size() - 1) instanceof GotoStatement) {
			score += 2;
		}
		
		//The last statement is a break or a goto or a continue
		if (statements.get(statements.size() - 1) instanceof BreakStatement ||
						statements.get(statements.size() - 1) instanceof ContinueStatement) {
			//if(logger.isDebugEnabled()) logger.debug("Ends with break or goto");
			score += 1;
		}
		
		//WEAK: one of the two last statements in the compound contains one of the keywords
		//in errorKeywords. For the last one we also check that is not a return statement 
		//(because if it is a return statement we already check for the keywords in the
		//corresponding method
		ASTNode lastStmt = statements.get(statements.size() - 1);
		ASTNode secondLastStmt = null;
		
		int counter = 0;
		if(statements.size() > 1)
			secondLastStmt = statements.get(statements.size() - 2);
		for(String keyw : errorKeywords){
			if ((!(lastStmt instanceof ReturnStatement) && lastStmt.getEscapedCodeStr().toLowerCase().contains(keyw.toLowerCase())) || 
					(secondLastStmt != null && secondLastStmt.getEscapedCodeStr()
					.toLowerCase().contains(keyw.toLowerCase())))
				counter += 1;
		}
		if(counter != 0){
			if(logger.isDebugEnabled()) logger.debug("{} keywords in the last two statements.", counter);
			score += counter;
		}
		
		//one of the two last statements in the compound passes the "free" check
		if (checkForFree(lastStmt.getEscapedCodeStr()) ||
				(secondLastStmt != null && checkForFree(secondLastStmt.getEscapedCodeStr()))){
			if(logger.isDebugEnabled()) logger.debug("Keyword present in last two statements");
			score += 1;
		}

			
		//WEAK: The last statement is not a return and not a goto, but it contains the RET keyword 
		if(!(statements.get(statements.size() - 1) instanceof ReturnStatement) &&
				!(statements.get(statements.size() - 1) instanceof GotoStatement) &&
				(statements.get(statements.size() - 1).getEscapedCodeStr().toLowerCase().contains(RET))){
			if(logger.isDebugEnabled()) logger.debug("Last statement is not a return and not a goto but contains \"ret\"");
			score += 1;
		}
		
		//The first statement is a label that contains an error keyword
		if(statements.get(0) instanceof Label &&
				errorKeywords.stream().anyMatch(keyw -> statements.get(0).getEscapedCodeStr().contains(keyw))){
			if(logger.isDebugEnabled()) logger.debug("First statement is an error-keyword matching label");
			score += 1;
		}
		
		//This generated too many false positives:
		// its length is less than or equal to errorCompoundLenght, the avg error blocks lenght  
//		if (statements.size() <= errorStmtListAvgLength)
//			score += 1;
		
		//TODO more heuristics and score adjustments?
		
		if(logger.isDebugEnabled()) logger.debug("> score {}", score);
		return score;

	}
	
	private int assignmentInitScore = 1;
	/**
	 * Returns true if the assignment matches one the initialization heuristics:
	 * 1) assigned value is one of the values in initValues;
	 * 2) the left part contains size and the right sizeof
	 * 3) only C constants in the right value, assuming the convention of upper case names is respected
	 * 4) the left part and the right part are "err" OR contain "_err_" or start with "err_" or end with "_err"
	 */
	public boolean isInit(AssignmentExpr assignment){
		return initHeuristicScore(assignment) >= assignmentInitScore;
	}
	
	private int initHeuristicScore(AssignmentExpr assignment) {
		int score = 0;

		
		//1) assignment value is 0, NULL
		if(initValues.stream().anyMatch(val -> val.equals(assignment.getAssignedValue()
				.getEscapedCodeStr().toLowerCase().replace(" ",""))))
			score++;
		
		//2) the left part contains size/len and the right sizeof
		if((assignment.getLeft().getEscapedCodeStr().toLowerCase().contains(SIZE) ||
				assignment.getLeft().getEscapedCodeStr().toLowerCase().endsWith(LENGTH) ||
				assignment.getLeft().getEscapedCodeStr().toLowerCase().endsWith("_" + LEN)) &&
				assignment.getAssignedValue().getEscapedCodeStr().toLowerCase().contains(SIZEOF))
			score++;
		
		//3) only C constants, assuming the convention of upper case names is respected
		List<ASTNode> identifiers = assignment.getAssignedValue().getNodes().stream().filter(n -> n instanceof Identifier).collect(Collectors.toList());
		if(identifiers.size() != 0 && identifiers.stream()
				.allMatch(id -> id.getEscapedCodeStr().toUpperCase().equals(id.getEscapedCodeStr())))
			score++;
		
		//4) the left part and the right part are "err" OR contain "_err_" or start with "err_" or end with "_err" 
		if((assignment.getLeft().getEscapedCodeStr().toLowerCase().equals(ERROR_KEYWORD) || 
				assignment.getLeft().getEscapedCodeStr().toLowerCase().endsWith("_" + ERROR_KEYWORD) ||
				assignment.getLeft().getEscapedCodeStr().toLowerCase().startsWith(ERROR_KEYWORD + "_") ||
				assignment.getLeft().getEscapedCodeStr().toLowerCase().contains("_" + ERROR_KEYWORD + "_")) &&
				(assignment.getAssignedValue().getEscapedCodeStr().toLowerCase().equals(ERROR_KEYWORD) || 
						assignment.getAssignedValue().getEscapedCodeStr().toLowerCase().endsWith("_" + ERROR_KEYWORD) ||
						assignment.getAssignedValue().getEscapedCodeStr().toLowerCase().startsWith(ERROR_KEYWORD + "_") ||
						assignment.getAssignedValue().getEscapedCodeStr().toLowerCase().contains("_" + ERROR_KEYWORD + "_")))
			score++;
		
		if(assignment.getLeft().getEscapedCodeStr().contains(".")) {
			score++;
		}
		
		if(assignment.getLeft() instanceof Identifier && assignment.getRight() instanceof Identifier) {
			score++;
		}
		
		
		//TODO more heuristics and score adjustments?
		
		return score;
	}
	
	private int expressionStatementInitScore = 1;
	
	/**
	 * Returns true if the statement matches one the initialization heuristics.
	 */
	public boolean isInit(Statement stmt){
		return initHeuristicScore(stmt) >= expressionStatementInitScore;
	}
	
	private int initHeuristicScore(Statement stmt) {
		if(logger.isDebugEnabled()) logger.debug("Checking init score for {}", stmt);
		int score = 0;
		
		if(stmt.getChild(0) instanceof AssignmentExpr &&
				isInit((AssignmentExpr) stmt.getChild(0))){
			score++;
		}
		
		if(stmt.getChild(0) instanceof Expression
				&& stmt.getChild(0).getChildCount() != 0
				&& stmt.getChild(0).getChildren().stream().allMatch(child -> child instanceof AssignmentExpr)){
			
			// We extract all the assignments
			List<AssignmentExpr> assignments = new ArrayList<AssignmentExpr>();
			stmt.getChild(0).getChildren().forEach(assgn -> assignments.add((AssignmentExpr) assgn));
			if(assignments.size() != 0 &&
					assignments.stream().allMatch(assgn -> isInit(assgn))){
				score++;
			}
		}
		
		if(stmt.getChild(0) instanceof CallExpression &&
				isInit((CallExpression) stmt.getChild(0))){
			score++;
		}
		
		if(stmt instanceof IdentifierDeclStatement){
			// We extract all the declarations
			List<IdentifierDecl> declarations = new ArrayList<IdentifierDecl>();
			stmt.getChildren().forEach(decl -> declarations.add((IdentifierDecl) decl));
			// We check if they are without assignment or if the assignment matches the heuristic
			if(declarations.size() != 0 &&
					declarations.stream().allMatch(decl ->
					(!(decl.getChild(decl.getChildCount() - 1) instanceof AssignmentExpr) ||
						((decl.getChild(decl.getChildCount() - 1) instanceof AssignmentExpr) &&
								isInit((AssignmentExpr) decl.getChild(decl.getChildCount() - 1)))))){
				score++;
			}
		}
		if(logger.isDebugEnabled()) logger.debug("> score {}", score);
		return score;
	}

	
	private int callInitScore = 1;

	private boolean isInit(CallExpression call){
		return initHeuristicScore(call) >= callInitScore;
	}
	
	private boolean isInitNew(CallExpression call){
		return initHeuristicScoreNew(call) >= callInitScore;
	}
	
	// new init statement checking.
	
	public boolean isInitNew(Statement stmt){
		return initHeuristicScoreNew(stmt) >= expressionStatementInitScore && !FlowRestrictiveMainConfig.ignoreInsertHeuristics;
	}
	
	private int initHeuristicScoreNew(Statement stmt) {
		if(logger.isDebugEnabled()) logger.debug("Checking init score for {}", stmt);
		int score = 0;
		
		if(stmt instanceof Label || stmt instanceof GotoStatement || 
				stmt instanceof ReturnStatement || 
				stmt instanceof BreakStatement || stmt instanceof ContinueStatement) {
			score++;
		}
		
		if(stmt.getChild(0) instanceof AssignmentExpr &&
				isInit((AssignmentExpr) stmt.getChild(0))){
			score++;
		}
		
		if(stmt.getChild(0) instanceof Expression
				&& stmt.getChild(0).getChildCount() != 0
				&& stmt.getChild(0).getChildren().stream().allMatch(child -> child instanceof AssignmentExpr)){
			
			// We extract all the assignments
			List<AssignmentExpr> assignments = new ArrayList<AssignmentExpr>();
			stmt.getChild(0).getChildren().forEach(assgn -> assignments.add((AssignmentExpr) assgn));
			if(assignments.size() != 0 &&
					assignments.stream().allMatch(assgn -> isInit(assgn))){
				score++;
			}
		}
		
		if(stmt.getChild(0) instanceof CallExpression &&
				isInitNew((CallExpression) stmt.getChild(0))){
			score++;
		}
		
		if(stmt instanceof IdentifierDeclStatement){
			// We extract all the declarations
			List<IdentifierDecl> declarations = new ArrayList<IdentifierDecl>();
			stmt.getChildren().forEach(decl -> declarations.add((IdentifierDecl) decl));
			// We check if they are without assignment or if the assignment matches the heuristic
			if(declarations.size() != 0 &&
					declarations.stream().allMatch(decl ->
					(!(decl.getChild(decl.getChildCount() - 1) instanceof AssignmentExpr) ||
						((decl.getChild(decl.getChildCount() - 1) instanceof AssignmentExpr) &&
								isInit((AssignmentExpr) decl.getChild(decl.getChildCount() - 1)))))){
				score++;
			}
		}
		if(logger.isDebugEnabled()) logger.debug("> score {}", score);
		return score;
	}

	
	private static final String SCANF = "scanf";
	private static final String PRINTF = "printf";
	public static final String LOCK = "lock";
	public static final String UNLOCK = "unlock";
	
	static final String[] knownMethodsArray = new String[] { "strcpy", "strncpy", "strlcpy", "memcpy", "fput" };
	private static final List<String> knownMethods = Arrays.asList(knownMethodsArray);
	
	static final String[] knownMethodsContainsArray = new String[] { "print", "assert" };
	
	private boolean callHasDangerousFormatSpecifiers(CallExpression call) {
		if(call.getChildCount() > 1) {
			// get the first argument
			ASTNode firstArg = call.getChild(1);
			return firstArg.toString().contains("%n");
		}
		return false;
	}
	
	private int initHeuristicScoreNew(CallExpression call) {
		int score = 0;
		
		String callee = call.getChild(0).getEscapedCodeStr().toLowerCase().trim();
		
		if(callee.equals(MEMSET)
				//&& call.getChild(1).getChild(1).getEscapedCodeStr().equals(ZERO)
					//&& call.getChild(1).getChild(2).getEscapedCodeStr().toLowerCase().contains(SIZE)
					)
			score++;
		
		// WEAK
		if(checkForFree(callee))
			score++;
		
		if(callee.startsWith("log_")) {
			// check if we have dangerous format specifiers.
			if(callHasDangerousFormatSpecifiers(call)) {
				logger.warn("Found a dangerous format specifier in call instruction.");
				// score is zero
				return 0;
			}
			
		}
			score++;
		
		
		if(knownMethods.contains(callee)) {
			score++;
		}
		
		if(callee.equals(SCANF) || callee.equals(PRINTF) || callee.equals(LOCK) || callee.equals(UNLOCK)) {
			score++;
			// if this is a printf call, check that the format string doesn't contain
			// %n, which changes the values.
			if(callee.equals(PRINTF) && callHasDangerousFormatSpecifiers(call)) {
				logger.warn("Found a dangerous format specifier in call instruction.");
				// score is zero
				return 0;
			}
		}
		if(callee.endsWith("_" + LOCK) || callee.endsWith("_" + UNLOCK) || 
				callee.startsWith(LOCK + "_") || callee.startsWith(UNLOCK + "_") || 
				callee.contains("_" + LOCK + "_") || callee.contains("_" + UNLOCK + "_")) {
			score++;
		}
		
		for(int i=0; i< knownMethodsContainsArray.length; i++) {
			if(callee.contains(knownMethodsContainsArray[i])) {
				score++;
			}
		}
		
		
		return score;
	}
	

	private int initHeuristicScore(CallExpression call) {
		int score = 0;
		
		if(call.getChild(0).getEscapedCodeStr().equals(MEMSET)
				//&& call.getChild(1).getChild(1).getEscapedCodeStr().equals(ZERO)
					//&& call.getChild(1).getChild(2).getEscapedCodeStr().toLowerCase().contains(SIZE)
					)
			score++;
		
		// WEAK
		if(checkForFree(call.getChild(0).getEscapedCodeStr()))
			score++;
		
		return score;
	}
	
	/**
	 * Returns true if all the statements in the list match the isInit method.
	 */
	public boolean isInit(List<Statement> l){
		return l.size() != 0 && l.stream().allMatch(stmt -> isInit((Statement) stmt));
	}
	
	
	private boolean checkForFree(String s){
		return s.toLowerCase().contains(FREE + "_") ||
				s.toLowerCase().endsWith(FREE) ||
				s.toLowerCase().contains("_" + FREE + "_");
	}
	
	/**
	 * Returns true if the CallExpression is a
	 * printing/logging method.
	 */
	public boolean isOutputMethod(CallExpression method){
		ArgumentList args = null;
		if(method.getChild(1) instanceof ArgumentList)
			args = (ArgumentList) method.getChild(1);
		else return false; // This should never happen
		
		if(args == null || args.getChildCount() == 0) return false;
		
		// Let's check if it has a string as argument
		Argument formatStringArg = null;
		
		for(ASTNode arg : args.getChildren()){
			ASTNode content = arg.getChild(0);
			if(content instanceof PrimaryExpression &&
					content.getEscapedCodeStr().startsWith("\"") &&
					content.getEscapedCodeStr().endsWith("\""))	 formatStringArg = (Argument) arg; 
		}
		
		// If we found the string...
		if(formatStringArg != null){
			//Let's count perc symbols excluding %%
			Matcher m = Pattern.compile("((?<!%)%(?!%))").matcher(formatStringArg.getEscapedCodeStr());
			int perc_counter = 0;
			while (m.find()) {
			    perc_counter++;
			}
			
			// If there are not perc singns it's output
			if(perc_counter == 0) return true;
			else {
				//We check further args, there must be enough for the percents
				if(1 + formatStringArg.getChildNumber() + perc_counter == args.getChildCount()) return true;
			}
		} else return false;
		
		return false;
	}
	
	/**
	 * Returns true if one of the following is true:
	 * 1) There are only deleted assignments
	 * 2) There is only deleted stuff
	 */
	public boolean onlyKnownDeletesPatterns(ASTDiff diff) {
		if(logger.isDebugEnabled()) logger.debug("Checking if it only matches one of the known delete patterns.");
		List<ASTNode> newNodes = diff.getNewNodesByAnyActionType();
		if(!newNodes.isEmpty()) return false;
		
		List<ASTNode> origNodes = diff.getOriginalNodesByAnyActionType();
		Set<ASTNode> assignments = new HashSet<ASTNode>(diff.getOriginalAffectedNodes(AssignmentExpr.class, ACTION_TYPE.DELETE));
		if(!assignments.isEmpty() &&
				origNodes.stream().allMatch(n -> ((n instanceof ExpressionStatement && diff.isDeleted(n)) ||
						assignments.stream().anyMatch(assgn -> assgn.getNodes().contains(n))))){
			if(logger.isDebugEnabled()) logger.debug("Only deleted known patterns.");
			return true;
		}
		
		//Checking if it's only deleted stuff		
		if(diff.getOriginalNodesByActionType(ACTION_TYPE.DELETE).equals(origNodes)){
			if(logger.isDebugEnabled()) logger.debug("Only deleted stuff.");
			return true;
		}
		
		return false;
	}

	/**
	 * Returns true if one of the following is true:
	 * 1) There are only inserted "-1"
	 */
	public boolean onlyKnownInsertionsPatterns(ASTDiff diff) {
		if(logger.isDebugEnabled()) logger.debug("Checking if it only matches one of the known insertion patterns.");
		List<ASTNode> origNodes = diff.getOriginalNodesByAnyActionType();
		List<ASTNode> newNodes = diff.getNewNodesByAnyActionType();
		
		if(newNodes.size() == 0) return false;
		Set<ASTNode> exprs = new HashSet<ASTNode>(diff.getNewAffectedNodes(AdditiveExpression.class, ACTION_TYPE.INSERT));

		if(!exprs.isEmpty() 
				&& exprs.stream().allMatch(expr -> (((AdditiveExpression) expr).getOperatorCode().equals("-") &&
				expr.getChild(1).getEscapedCodeStr().equals("1") &&
				(origNodes.isEmpty() || 
						origNodes.stream().allMatch(n -> expr.getChild(0).getNodes().stream()
								.anyMatch(n1 -> n.getEscapedCodeStr().equals(n1.getEscapedCodeStr()))))))){
			if(logger.isDebugEnabled()) logger.debug("Added -1 to an AdditiveExpression");
			return true;
		}
		
		return false;
	}
	
	/**
	 * Returns true if all the statements
	 * are output/debug/printing statements
	 * (Joern parsing errors are also considered in these) 
	 */
	public boolean outputOnly(Set<ASTNode> affectedStatements){
		boolean retval = affectedStatements.size() != 0 
				&& affectedStatements.stream()
				.allMatch(stmt -> ((stmt instanceof Statement) &&
						(stmt instanceof ExpressionStatement) &&
						(stmt.getChild(0) instanceof CallExpression) &&
						!checkForFree(stmt.getChild(0).getChild(0).getEscapedCodeStr()) &&
						(this.isOutputMethod((CallExpression) stmt.getChild(0)) ||
						this.parsingError((Statement) stmt))));
		if(retval)
			if(logger.isDebugEnabled()) logger.debug("Output only - true");
		return retval;
	}
	
	private boolean parsingError(Statement stmt){
		// When Joern cannot part something it just perse every
		// component of the unparsed region as an unspecified Statement,
		// resulting in a set of "Statement" objects
		return stmt.getClass().getSimpleName().equals("Statement") ||
				stmt.getEscapedCodeStr().length() == 0;
	}
	
	private static final String MINUS_SIGN = "-";
	private static final String ERROR_KEYWORD = "err";
	private static final String MEMSET = "memset";
	private static final String FREE = "free";
	private static final String SIZE = "size";
	private static final String SIZEOF = "sizeof";
	private static final String RET = "ret";
	private static final String NULL = "null";
	private static final String FALSE = "false";
	private static final String LEN = "len";
	private static final String LENGTH = "length";
	
	static final String[] errorCodesArray = new String[] { "ESRMNT", "ELIBSCN",
			"EWOULDBLOCK", "EUSERS", "EALREADY", "ECONNREFUSED", "EISDIR",
			"EXDEV", "EOWNERDEAD", "EMFILE", "ENAVAIL", "EACCES", "ENOLINK",
			"EINPROGRESS", "ENFILE", "EADDRINUSE", "ENONET", "EL3RST", "ESTALE",
			"ENOTCONN", "EISNAM", "ECONNABORTED", "EROFS", "ERESTART", "EFAULT",
			"EDEADLOCK", "ETXTBSY", "EUNATCH", "EEXIST", "ESPIPE", "ENOSYS",
			"EBADE", "EBADF", "EISCONN", "ENOSR", "ENOPROTOOPT", "EBADR",
			"ENODEV", "EBADSLT", "E2BIG", "ENOPKG", "ENOTUNIQ", "ELNRNG",
			"ENOTTY", "ENETDOWN", "EPROTOTYPE", "ENOSPC", "EMEDIUMTYPE",
			"ELOOP", "ENODATA", "EMLINK", "EL2NSYNC", "EKEYREJECTED",
			"EAFNOSUPPORT", "ESTRPIPE", "ENOLCK", "ELIBMAX", "ENOKEY", "EPIPE",
			"EREMOTE", "ESOCKTNOSUPPORT", "ENOTNAM", "ENOEXEC", "EADV",
			"EINVAL", "EL3HLT", "ELIBBAD", "ENOMEDIUM", "EDESTADDRREQ",
			"ENETUNREACH", "ENOBUFS", "ELIBACC", "EDEADLK", "EBFONT", "EINTR",
			"ENAMETOOLONG", "ENOTRECOVERABLE", "ETIMEDOUT", "ESHUTDOWN",
			"EBADFD", "ECONNRESET", "EREMOTEIO", "ENOANO", "ENOTBLK",
			"EKEYEXPIRED", "EADDRNOTAVAIL", "EIDRM", "ENXIO", "ENOTDIR",
			"EKEYREVOKED", "EHOSTDOWN", "ETIME", "ENOENT", "EBADMSG", "EIO",
			"EPROTONOSUPPORT", "EPROTO", "EUCLEAN", "EDOTDOT", "ENETRESET",
			"ENOMSG", "EHOSTUNREACH", "ENOCSI", "ESRCH", "EDOM", "EXFULL",
			"ENOSTR", "EFBIG", "EBADRQC", "ECANCELED", "ETOOMANYREFS", "EILSEQ",
			"EOPNOTSUPP", "EAGAIN", "EPFNOSUPPORT", "ECHRNG", "EL2HLT",
			"ELIBEXEC", "ERANGE", "ECOMM", "ENOMEM", "ENOTSOCK", "EOVERFLOW",
			"ENOTEMPTY", "ECHILD", "EPERM", "EMULTIHOP", "EBUSY", "EDQUOT",
			"EREMCHG", "EMSGSIZE" };
	private static final List<String> errorCodes = Arrays.asList(errorCodesArray);
	
	//TODO check keywords, some of these can lead to false positives
	static final String[] errorKeywordsArray = new String[] { "error", "warn", "print", "check", "exit", "inval",
			"fail", "panic", "longjmp", "fatal", "illegal" };

	private static final List<String> errorKeywords = Arrays.asList(errorKeywordsArray);

	static final String[] initValuesArray = new String[] { "0", NULL, /*"-1", "1" TODO: needed? check*/ };
	private static final List<String> initValues = Arrays.asList(initValuesArray);
	
	public static List<String> getErrorcodes() {
		return errorCodes;
	}

	public static List<String> getErrorkeywords() {
		return errorKeywords;
	}
	
	
}

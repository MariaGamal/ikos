/*******************************************************************************
 *
 * \file
 * \brief Division by zero checker implementation
 *
 * Author: Maxime Arthaud
 *
 * Contact: ikos@lists.nasa.gov
 *
 * Notices:
 *
 * Copyright (c) 2011-2019 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Disclaimers:
 *
 * No Warranty: THE SUBJECT SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY OF
 * ANY KIND, EITHER EXPRESSED, IMPLIED, OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL CONFORM TO SPECIFICATIONS,
 * ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * OR FREEDOM FROM INFRINGEMENT, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL BE
 * ERROR FREE, OR ANY WARRANTY THAT DOCUMENTATION, IF PROVIDED, WILL CONFORM TO
 * THE SUBJECT SOFTWARE. THIS AGREEMENT DOES NOT, IN ANY MANNER, CONSTITUTE AN
 * ENDORSEMENT BY GOVERNMENT AGENCY OR ANY PRIOR RECIPIENT OF ANY RESULTS,
 * RESULTING DESIGNS, HARDWARE, SOFTWARE PRODUCTS OR ANY OTHER APPLICATIONS
 * RESULTING FROM USE OF THE SUBJECT SOFTWARE.  FURTHER, GOVERNMENT AGENCY
 * DISCLAIMS ALL WARRANTIES AND LIABILITIES REGARDING THIRD-PARTY SOFTWARE,
 * IF PRESENT IN THE ORIGINAL SOFTWARE, AND DISTRIBUTES IT "AS IS."
 *
 * Waiver and Indemnity:  RECIPIENT AGREES TO WAIVE ANY AND ALL CLAIMS AGAINST
 * THE UNITED STATES GOVERNMENT, ITS CONTRACTORS AND SUBCONTRACTORS, AS WELL
 * AS ANY PRIOR RECIPIENT.  IF RECIPIENT'S USE OF THE SUBJECT SOFTWARE RESULTS
 * IN ANY LIABILITIES, DEMANDS, DAMAGES, EXPENSES OR LOSSES ARISING FROM SUCH
 * USE, INCLUDING ANY DAMAGES FROM PRODUCTS BASED ON, OR RESULTING FROM,
 * RECIPIENT'S USE OF THE SUBJECT SOFTWARE, RECIPIENT SHALL INDEMNIFY AND HOLD
 * HARMLESS THE UNITED STATES GOVERNMENT, ITS CONTRACTORS AND SUBCONTRACTORS,
 * AS WELL AS ANY PRIOR RECIPIENT, TO THE EXTENT PERMITTED BY LAW.
 * RECIPIENT'S SOLE REMEDY FOR ANY SUCH MATTER SHALL BE THE IMMEDIATE,
 * UNILATERAL TERMINATION OF THIS AGREEMENT.
 *
 ******************************************************************************/

#include <set>
#include <regex>
// ADDED INCLUDES START
#include <fstream>
#include <iostream>
// ADDED INCLUDES END

#include <ikos/analyzer/analysis/literal.hpp>
#include <ikos/analyzer/checker/division_by_zero.hpp>
#include <ikos/analyzer/json/helper.hpp>
#include <ikos/analyzer/support/cast.hpp>
#include <ikos/analyzer/util/log.hpp>
#include <ikos/analyzer/util/source_location.hpp>

// ADDED INCLUDES START
#include <llvm/Support/CommandLine.h>
#include <ikos/analyzer/support/reader.hpp>
#include <sstream>
#include <unordered_map>
#include "ikos/analyzer/analysis/value/machine_int_domain.hpp"
#include "ikos/ar/semantic/statement.hpp"
#include "ikos/ar/semantic/value.hpp"
#include "ikos/core/domain/numeric/apron.hpp"
#include "ikos/core/number/z_number.hpp"
#include "ikos/core/value/machine_int/interval_congruence.hpp"
// ADDED INCLUDES END

namespace ikos {
namespace analyzer {

inline bool operator<(const Origin& lhs, const Origin& rhs)
{
  if ( (lhs.kind == ArrayKind) && (rhs.kind == ArrayKind) ) {
	  if (lhs.src == rhs.src) return lhs.index < rhs.index;
	  else return lhs.src < rhs.src;
  }
  return lhs.src < rhs.src;
}
// ADDED ARG START
static llvm::cl::OptionCategory GradCategory("Grad Options");
static llvm::cl::opt< std::string > DataFilename(
    "lines",
    llvm::cl::desc("Specify the name of the data file"),
    llvm::cl::value_desc("filename"),
    llvm::cl::cat(GradCategory));

std::string read_file(const std::string& file_path) {
  std::ifstream file(file_path);
  if (!file.is_open()) {
    std::cerr << "Failed to open file: " << file_path << std::endl;
    return "";
  }

  std::string content((std::istreambuf_iterator< char >(file)),
                      std::istreambuf_iterator< char >());
  file.close();

  return content;
}
// ADDED ARG END

std::vector<struct Constraint> getConstraint(std::string cons, ar::Value* arg1, ar::Value* arg2, std::string val1, std::string val2) {
	std::string regstr = "-?" + val1 + "[-+]" + val2 + " <= -?[[:digit:]]+";
	//std::cout << "\n" << regstr << "\n";
		//"-?\\%0x[[:digit:]]+[-+]\\%0x[[:digit:]]+ <= -?[[:digit:]]+";
	std::regex regexp(regstr);
	std::smatch m;
	std::vector<struct Constraint> constraints;
	while (std::regex_search(cons, m, regexp)) {
		for (auto x: m) {
			//std::cout << "XX " << x.str() << "\n"; 
			constraints.push_back(parseConstraint(x.str(), arg1, arg2, val1, val2));
		}
		//std::cout << "\n";
		cons = m.suffix().str();
	}
	return constraints;
}

struct Constraint parseConstraint(std::string cons, ar::Value* arg1, ar::Value* arg2, std::string val1, std::string val2) {
	//std::cout << cons << ": ";
	OpKind first_op = Plus;
	if (cons[0] == '-') {
		first_op = Minus;
		cons = cons.substr(1);
	}
	//std::cout << cons << " ";
	cons = cons.substr(val1.size());
	//std::cout << cons << " ";

	OpKind second_op = Plus;
	if (cons[0] == '-') {
		second_op = Minus;
	}

	cons = cons.substr(1+val2.size()+4);
	//std::cout << cons << "\n";

	int num = std::stoi(cons);

	struct Constraint constraint = {first_op, arg1, second_op, arg2, num};

	//std::cout << first_op << " ";
	//arg1->dump(std::cout);
	//std::cout << " " << second_op << " ";
	//arg2->dump(std::cout);
	//std::cout << " " << num << "\n";

	return constraint;
}

bool leq(std::string cons, ar::Value* arg1, ar::Value* arg2, std::string val1, std::string val2) {
	return leq(getConstraint(cons,arg1,arg2,val1,val2), arg1, arg2) || 
		   leq(getConstraint(cons,arg2,arg1,val2,val1), arg1, arg2);
}
bool le(std::string cons, ar::Value* arg1, ar::Value* arg2, std::string val1, std::string val2) {
	return le(getConstraint(cons,arg1,arg2,val1,val2), arg1, arg2) || 
		   le(getConstraint(cons,arg2,arg1,val2,val1), arg1, arg2);
}

bool leq(std::vector<struct Constraint> constraints, ar::Value* arg1, ar::Value* arg2) {
	for (auto cons: constraints) {
		if ( (arg1 == cons.first_arg) && 
			 (cons.first_op == Plus) && 
			 (cons.second_op == Minus) &&
			 (cons.num <= 0) ) {
			//std::cout << "FIRST: ";
			//std::cout << cons.first_op << " ";
			//cons.first_arg->dump(std::cout);
			//std::cout << " " << cons.second_op << " ";
			//cons.second_arg->dump(std::cout);
			//std::cout << " " << cons.num << "\n";
			return true;
		}
		if ( (arg1 == cons.second_arg) && 
			 (cons.second_op == Plus) && 
			 (cons.first_op == Minus) &&
			 (cons.num <= 0) ) {
			//std::cout << "SECOND: ";
			//std::cout << cons.first_op << " ";
			//cons.first_arg->dump(std::cout);
			//std::cout << " " << cons.second_op << " ";
			//cons.second_arg->dump(std::cout);
			//std::cout << " " << cons.num << "\n";
			return true;
		}
	}
	return false;
}

bool le(std::vector<struct Constraint> constraints, ar::Value* arg1, ar::Value* arg2) {
	for (auto cons: constraints) {
		if ( (arg1 == cons.first_arg) && 
			 (cons.first_op == Plus) && 
			 (cons.second_op == Minus) &&
			 (cons.num < 0) ) {
			return true;
		}
		if ( (arg1 == cons.second_arg) && 
			 (cons.second_op == Plus) && 
			 (cons.first_op == Minus) &&
			 (cons.num < 0) ) {
			return true;
		}
	}
	return false;
}

DivisionByZeroChecker::DivisionByZeroChecker(Context& ctx) : Checker(ctx) {}

CheckerName DivisionByZeroChecker::name() const {
  return CheckerName::DivisionByZero;
}

const char* DivisionByZeroChecker::description() const {
  return "Division by zero checker";
}

bool parsed = false;

struct Loop {
  int first_part_start;
  int first_part_end;
  int second_part_start;
  int second_part_end;
  std::vector< ar::Statement* > first_part_stmts;
  //std::vector< value::AbstractDomain& > first_part_inv;
  // This might not be needed
  std::vector< ar::Statement* > second_part_stmts;
};

std::vector< struct Loop > loops;
std::unordered_map<ar::Statement*,const value::AbstractDomain*> inv_at_stmt;

void printSet(std::set<struct Origin> s) {
  for(auto it: s) {
	  if (it.kind == ArrayKind) {
		  std::cout << "Array ";
		  it.src->dump(std::cout);
		  std::cout << " ";
		  it.index->dump(std::cout);
		  std::cout << ", ";
	  } else if (it.kind == ConstantKind){
		  std::cout << "Constant ";
		  it.src->dump(std::cout);
		  std::cout << ", ";
	  } else {
		  std::cout << "Variable ";
		  it.src->dump(std::cout);
		  std::cout << ", ";
	  }
  }
}


void DivisionByZeroChecker::check(ar::Statement* stmt,
                                  const value::AbstractDomain& inv,
                                  CallContext* call_context) {
  if (!parsed) {
    this->parseMetaFile(&loops, DataFilename);
    parsed = true;
  }
  inv_at_stmt.insert({stmt, &inv});
  //inv.normal().runtime_domain();

  
  //auto foo = dyn_cast<core::numeric::ApronDomain<core::numeric::apron::Octagon, core::ZNumber, ar::Variable*>>(&inv.normal());

  //stmt->dump(std::cout);
  //if ( (stmt->kind() == ar::Statement::StoreKind))
	   ////(stmt->kind() == ar::Statement::LoadKind)  ||
	   //{
	  //std::cout << ":  \n";
	  //for(int i = 0; i < stmt->num_operands(); ++i) {
		  //std::cout << "    ";
		  //ar::Value *v = stmt->operand(i);
		  //v->dump(std::cout);
		  //std::cout << ": " << v->kind() << " - ";
		  //std::set<struct Origin> s = this->getValueOrigin(v,stmt);
		  //printSet(s);
		  //std::cout << "\n";
	  //}
	  //std::cout << "\n\n";
  //}
  //else {
	  //std::cout << "\n";
  //}


  //inv.to_linear_constraint_system;
  stmt->dump(std::cout);
  std::cout << "\n";
  
  if (!this->isTarget(stmt))
    return;
  //stmt->dump(std::cout);
  //if ( (stmt->kind() == ar::Statement::StoreKind))
	   ////(stmt->kind() == ar::Statement::LoadKind)  ||
	   //{
	  //std::cout << ":  \n";
	  //for(int i = 0; i < stmt->num_operands(); ++i) {
		  //std::cout << "    ";
		  //ar::Value *v = stmt->operand(i);
		  //v->dump(std::cout);
		  //std::cout << ": " << v->kind() << " - ";
		  //std::set<struct Origin> s = this->getValueOrigin(v,stmt);
		  //printSet(s);
		  //std::cout << "\n";
	  //}
	  //std::cout << "\n\n";
  //}
  //else {
	  //std::cout << "\n";
  //}
  //
  //stmt->dump(std::cout);

  std::pair< bool, int > is_first_part = this->isFirstPart(stmt);
  std::pair< bool, int > is_second_part = this->isSecondPart(stmt);

  //std::cout << " " << is_first_part.first << " " << is_second_part.first << "\n";

  if (is_first_part.first)
	this->handleFirstPartStmt(stmt, is_first_part.second);
  else if (is_second_part.first) {
	//inv.dump(std::cout);
	//std::cout << "\n\n\n\n\n\n\n";
	//stmt->dump(std::cout);
	//std::cout << "\n";
	this->handleSecondPartStmt(stmt, inv, is_second_part.second);
  }

  if (auto bin = dyn_cast< ar::BinaryOperation >(stmt)) {
    if (bin->op() == ar::BinaryOperation::UDiv ||
        bin->op() == ar::BinaryOperation::SDiv ||
        bin->op() == ar::BinaryOperation::URem ||
        bin->op() == ar::BinaryOperation::SRem) {
      CheckResult check = this->check_division(bin, inv);
      this->display_invariant(check.result, stmt, inv);
      this->_checks.insert(check.kind,
                           CheckerName::DivisionByZero,
                           check.result,
                           stmt,
                           call_context,
                           std::array< ar::Value*, 1 >{{bin->right()}},
                           check.info);
    }
  }
}

DivisionByZeroChecker::CheckResult DivisionByZeroChecker::check_division(
    ar::BinaryOperation* stmt, const value::AbstractDomain& inv) {
  if (inv.is_normal_flow_bottom()) {
    // Statement unreachable
    if (auto msg = this->display_division_check(Result::Unreachable, stmt)) {
      *msg << "\n";
    }
    return {CheckKind::Unreachable, Result::Unreachable, {}};
  }

  const ScalarLit& lit = this->_lit_factory.get_scalar(stmt->right());

  if (lit.is_undefined() || (lit.is_machine_int_var() &&
                             inv.normal().uninit_is_uninitialized(lit.var()))) {
    // Undefined operand
    if (auto msg = this->display_division_check(Result::Error, stmt)) {
      *msg << ": undefined operand\n";
    }
    return {CheckKind::UninitializedVariable, Result::Error, {}};
  }

  auto divisor = IntInterval::bottom(1, Signed);
  if (lit.is_machine_int()) {
    divisor = IntInterval(lit.machine_int());
  } else if (lit.is_machine_int_var()) {
    divisor = inv.normal().int_to_interval(lit.var());
  } else {
    log::error("unexpected operand to binary operation");
    return {CheckKind::UnexpectedOperand, Result::Error, {}};
  }

  boost::optional< MachineInt > d = divisor.singleton();

  if (d && (*d).is_zero()) {
    // The second operand is definitely 0
    if (auto msg = this->display_division_check(Result::Error, stmt)) {
      *msg << ": ∀d ∈ divisor, d == 0\n";
    }
    return {CheckKind::DivisionByZero, Result::Error, {}};
  } else if (divisor.contains(
                 MachineInt::zero(divisor.bit_width(), divisor.sign()))) {
    // The second operand may be 0
    if (auto msg = this->display_division_check(Result::Warning, stmt)) {
      *msg << ": ∃d ∈ divisor, d == 0\n";
    }
    return {CheckKind::DivisionByZero, Result::Warning, to_json(divisor)};
  } else {
    // The second operand cannot be definitely 0
    if (auto msg = this->display_division_check(Result::Ok, stmt)) {
      *msg << ": ∀d ∈ divisor, d != 0\n";
    }
    return {CheckKind::DivisionByZero, Result::Ok, {}};
  }
}

void DivisionByZeroChecker::parseMetaFile(std::vector< struct Loop >* loops,
                                          std::string filename) {
  std::string content = read_file(filename);

  std::stringstream string_stream(content);
  std::string line;
  while (std::getline(string_stream, line, '\n')) {
    std::stringstream string_stream_2(line);
    std::string num;
    struct Loop loop;

    std::getline(string_stream_2, num, ' ');
    loop.first_part_start = std::stoi(num);

    std::getline(string_stream_2, num, ' ');
    loop.first_part_end = std::stoi(num);

    std::getline(string_stream_2, num, ' ');
    loop.second_part_start = std::stoi(num);

    std::getline(string_stream_2, num, ' ');
    loop.second_part_end = std::stoi(num);

    loops->push_back(loop);
  }
}

// Check if stmt is load or store inside a target loop
bool DivisionByZeroChecker::isTarget(ar::Statement* stmt) {
  if (stmt->kind() == ar::Statement::StoreKind) {
    return true;
  }
  return false;
}

std::pair< bool, int > DivisionByZeroChecker::isFirstPart(ar::Statement* stmt) {
  SourceLocation source = source_location(stmt);
  unsigned int line = source.line();
  for (int loop_index = 0; loop_index < loops.size(); loop_index++) {
    if (line >= loops[loop_index].first_part_start &&
        line <= loops[loop_index].first_part_end) {
      return {true, loop_index};
    }
  }
  return {false, -1};
}

// Just store statement in appropriate Loop struct in loops
void DivisionByZeroChecker::handleFirstPartStmt(ar::Statement* stmt,
                                                int loop_index) {
  loops[loop_index].first_part_stmts.push_back(stmt);
}

std::pair< bool, int > DivisionByZeroChecker::isSecondPart(ar::Statement* stmt) {
  SourceLocation source = source_location(stmt);
  unsigned int line = source.line();
  for (int loop_index = 0; loop_index < loops.size(); loop_index++) {
    if (line >= loops[loop_index].second_part_start &&
        line <= loops[loop_index].second_part_end) {
      return {true, loop_index};
    }
  }
  return {false, -1};
}

// TODO
// Compare with stored statements
void DivisionByZeroChecker::handleSecondPartStmt(
    ar::Statement* stmt,
    const value::AbstractDomain& inv,
    int loop_index) {
	assert(stmt->kind() == ar::Statement::StoreKind);

	std::cout << "STMT: ";
	stmt->dump(std::cout);
	SourceLocation source = source_location(stmt);
	std::cout << " (" << source.line() << ")";

	ar::Value *write = stmt->operand(0);
	ar::Value *read  = stmt->operand(1);

	//std::cout << "*****************\n";
	//write->dump(std::cout);
	//std::cout << "\n\n\n";

	std::set<struct Origin> write_origin = this->getValueOrigin(write, stmt);
	std::set<struct Origin> read_origin  = this->getValueOrigin(read, stmt);

	std::cout << " - ";
	printSet(read_origin);
	std::cout << "  --  ";
	printSet(write_origin);
	std::cout << "\n";
	
	for (int i = 0; i < loops[loop_index].first_part_stmts.size(); i++) {
		ar::Statement* cur_stmt = loops[loop_index].first_part_stmts[i];
		std::cout << "CUR STMT: ";
		cur_stmt->dump(std::cout);
  		SourceLocation source = source_location(cur_stmt);
		std::cout << " (" << source.line() << ")";
		assert(cur_stmt->kind() == ar::Statement::StoreKind);

		ar::Value *cur_write = cur_stmt->operand(0);
		ar::Value *cur_read  = cur_stmt->operand(1);


		std::set<struct Origin> cur_write_origin = this->getValueOrigin(cur_write, cur_stmt);
		std::set<struct Origin> cur_read_origin  = this->getValueOrigin(cur_read, cur_stmt);
		std::cout << " - ";
		printSet(cur_read_origin);
		std::cout << "  --  ";
		printSet(cur_write_origin);
		std::cout << "\n";
		
		for (auto it: cur_read_origin) {
			if (it.kind != ArrayKind) {
				//std::cout << "\n" << it.kind << "\n";
				continue;
			}
			for (auto it2: cur_write_origin) {
				if ( (it2.kind != ArrayKind) || (it.src != it2.src) ) {
					//std::cout << "\n" << it2.kind << " " << it.src << " " << it2.src << "\n";
					continue;
				}

				std::stringstream s, n1, n2;
				inv_at_stmt[cur_stmt]->dump(s);
				it.index->dump(n1);
				it2.index->dump(n2);
				std::string cons = s.str();
				std::string var1 = n1.str();
				std::string var2 = n2.str();

				//cur_stmt->dump(s);
				//std::cout << " ";
				it.index->dump(std::cout);
				std::cout << " ";
				it2.index->dump(std::cout);
				std::cout << " " << leq(cons, it.index, it2.index, var1, var2) << "\n";

			}
		}

		for (auto it: read_origin) {
			if (it.kind == ConstantKind) {
				continue;
			}
			for (auto it2: cur_write_origin) {
				if (it.kind != it2.kind) {
					continue;
				}
				if ( (it.kind == ArrayKind) && (it.src == it2.src) ) {
					std::stringstream s, n1, n2;
					inv.dump(s);
					it.index->dump(n1);
					it2.index->dump(n2);
					std::string cons = s.str();
					std::string var1 = n1.str();
					std::string var2 = n2.str();

					it.index->dump(std::cout);
					std::cout << " ";
					it2.index->dump(std::cout);
					std::cout << " " << leq(cons, it2.index, it.index, var2, var1) << "\n";
				}
			}
		}




				//const ScalarLit& read_lit = this->_lit_factory.get_scalar(it.index);
				//const ScalarLit& write_lit = this->_lit_factory.get_scalar(it2.index);
				////auto foo = value::MachineIntAbstractDomain(inv.normal());

				//auto read_ar = core::machine_int::IntervalCongruence::bottom(1, Signed);
				//auto write_ar = core::machine_int::IntervalCongruence::bottom(1, Signed);
				//if (read_lit.is_machine_int()) {
					//read_ar = core::machine_int::IntervalCongruence(read_lit.machine_int());
				//} else if (read_lit.is_machine_int_var()) {
					//auto cinv = *inv_at_stmt[it.stmt];
					//read_ar = cinv.normal().int_to_interval_congruence(read_lit.var());
				//} else {
					//std::cout << "\n" << "WTH IS THIS" << "\n";
				//}

				//if (write_lit.is_machine_int()) {
					//write_ar = core::machine_int::IntervalCongruence(write_lit.machine_int());
				//} else if (read_lit.is_machine_int_var()) {
					//auto cinv = *inv_at_stmt[it.stmt];
					//write_ar = cinv.normal().int_to_interval_congruence(write_lit.var());
				//} else {
					//std::cout << "\n" << "WTH IS THIS" << "\n";
				//}

				//read_ar.dump(std::cout);
				//std::cout << "\n";
				//write_ar.dump(std::cout);
				//std::cout << "\n";
				//cur_stmt->dump(std::cout);
				//std::cout << " " << read_ar.leq(write_ar) << "\n";
	}
}

// Get the origin(s) of a variable
std::set<struct Origin> DivisionByZeroChecker::getValueOrigin(ar::Value* value, ar::Statement* stmt) {
	std::set<struct Origin> origins;

	ar::Variable* var = dyn_cast<ar::Variable>(value);
	if (value->is_constant()) {
	  origins.insert({ConstantKind, value, nullptr, stmt});
	}
	else if (value->is_global_variable() || value->is_local_variable()) {
	  origins.insert({VariableKind, value, nullptr, stmt});
	}
	else if ( (stmt->result_or_null() == var) ) {
      if (stmt->kind() == ar::Statement::PointerShiftKind) { 
	    origins.insert({ArrayKind, stmt->operand(0), stmt->operand(2), stmt});
	  }
	  else {
		for (int i = 0; i < stmt->num_operands(); ++i) {
		  std::set<struct Origin> cur_set = getValueOrigin(stmt->operand(i), stmt);
		  for (auto it: cur_set) {
			origins.insert(it);
		  }
		}
	  }
	}
	else {
	  origins = getValueOrigin(value, stmt->prev_statement());
	}
	return origins;
}

llvm::Optional< LogMessage > DivisionByZeroChecker::display_division_check(
    Result result, ar::BinaryOperation* stmt) const {
  auto msg = this->display_check(result, stmt);
  if (msg) {
    *msg << "check_dbz(";
    stmt->dump(msg->stream());
    *msg << ")";
  }
  return msg;
}

} // end namespace analyzer
} // end namespace ikos

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
// ADDED INCLUDES END

namespace ikos {
namespace analyzer {

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
  // This might not be needed
  std::vector< ar::Statement* > second_part_stmts;
};

std::vector< struct Loop > loops;

void DivisionByZeroChecker::check(ar::Statement* stmt,
                                  const value::AbstractDomain& inv,
                                  CallContext* call_context) {
  if (!parsed) {
    this->parseMetaFile(&loops, DataFilename);
    parsed = true;
  }

  if (!this->isTarget(stmt))
    return;

  std::pair< bool, int > is_first_part = this->isFirstPart(stmt);

  if (is_first_part.first)
    this->handleFirstPartStmt(stmt, is_first_part.second);
  else
    this->handleSecondPartStmt(stmt, inv, call_context);

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

// Check if stmt is store, load or assignment inside a target loop
bool DivisionByZeroChecker::isTarget(ar::Statement* stmt) {
  // load = 8, store = 9, assignment = 0
  if (stmt->kind() == 8 || stmt->kind() == 9) {
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

// TODO
// Compare with stored statements
void DivisionByZeroChecker::handleSecondPartStmt(
    ar::Statement* stmt,
    const value::AbstractDomain& inv,
    CallContext* call_context) {}

// TODO
// Get the statement where the variable/value is defined
// This will probably be needed in handleSecondPartStmt function
// Not sure for now if it's needed for Value, Variable, or both
// ar::Statement* getValueOrigin(ar::Value* stmt) {}
// ar::Statement* getVariableOrigin(ar::Variable* stmt) {}

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

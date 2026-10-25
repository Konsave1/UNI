#include "personnel.h"
#include "sequence.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <deque>
#include <fstream>
#include <iostream>
#include <iterator>
#include <list>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string scratchDirectory;
unsigned int fileNumber = 0;

std::string numberText(std::size_t value) {
    std::ostringstream text;
    text << value;
    return text.str();
}

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void requireSuccess(bool result, const std::string& error,
                    const std::string& message) {
    require(result, message + (error.empty() ? "" : ": " + error));
}

void requireFailure(bool result, const std::string& error,
                    const std::string& message) {
    require(!result, message + " unexpectedly succeeded");
    require(!error.empty(), message + " did not explain the failure");
}

std::string newPath(const std::string& label) {
    std::ostringstream name;
    name << scratchDirectory << "/dz1_test_" << ++fileNumber << '_' << label;
    return name.str();
}

void writeFixture(const std::string& path, const std::string& bytes) {
    std::ofstream output(path.c_str(), std::ios::binary);
    require(static_cast<bool>(output), "cannot create fixture " + path);
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    output.close();
    require(static_cast<bool>(output), "cannot finish fixture " + path);
}

std::string readBytes(const std::string& path) {
    std::ifstream input(path.c_str(), std::ios::binary);
    require(static_cast<bool>(input), "cannot read " + path);
    std::string bytes;
    char byte = 0;
    while (input.get(byte)) {
        bytes += byte;
    }
    require(!input.bad(), "cannot finish reading " + path);
    return bytes;
}

bool near(double left, double right) {
    return std::isfinite(left) && std::isfinite(right) &&
           std::fabs(left - right) <=
           1e-9 * std::max(1.0, std::max(std::fabs(left), std::fabs(right)));
}

template <typename Container>
void requireNumbers(const Container& actual,
                    const std::vector<double>& expected,
                    const std::string& message) {
    require(actual.size() == expected.size(), message + ": wrong size");
    typename Container::const_iterator current = actual.begin();
    for (std::size_t index = 0; index < expected.size(); ++index, ++current) {
        require(near(static_cast<double>(*current), expected[index]),
                message + ": wrong value at " + numberText(index));
    }
}

std::vector<double> readTextNumbers(const std::string& path) {
    std::ifstream input(path.c_str());
    require(static_cast<bool>(input), "cannot open numeric output " + path);
    std::vector<double> values;
    std::string token;
    while (input >> token) {
        std::istringstream parser(token);
        double value = 0;
        require(static_cast<bool>(parser >> value) && std::isfinite(value) &&
                parser.peek() == std::char_traits<char>::eof(),
                "numeric output contains an invalid number");
        values.push_back(value);
    }
    require(!input.bad() && input.eof(), "cannot finish reading numeric output");
    return values;
}

void requireReportTotal(const std::string& text, const std::string& label,
                        double expected) {
    const std::string::size_type position = text.find(label);
    require(position != std::string::npos, "report is missing " + label);
    std::istringstream valueText(text.substr(position + label.size()));
    double actual = 0;
    require(static_cast<bool>(valueText >> actual) && near(actual, expected),
            "wrong report total after " + label);
}

template <typename Input, typename Output>
void checkTransformation(const std::vector<int>& values,
                         const std::vector<double>& expected,
                         const std::string& label) {
    const Input input(values.begin(), values.end());
    const Input original(input);
    std::string error;
    Output containerResult;
    requireSuccess(sequence::modifyContainer(input, containerResult, error),
                   error, label + " container transform");
    requireNumbers(containerResult, expected, label + " container result");
    require(input == original, label + " container changed the input");

    Output iteratorResult;
    requireSuccess(sequence::modifyIterators(input.begin(), input.end(),
                   std::back_inserter(iteratorResult), error),
                   error, label + " iterator transform");
    const std::vector<double> transformed(expected.begin(), expected.end() - 2);
    requireNumbers(iteratorResult, transformed,
                   label + " iterator must emit only the transformed elements");
    requireSuccess(sequence::appendStatistics(iteratorResult, error), error,
                   label + " iterator statistics");
    requireNumbers(iteratorResult, expected, label + " iterator result");
    require(input == original, label + " iterators changed the input");

    Output forEachResult;
    requireSuccess(sequence::modifyForEach(input, forEachResult, error), error,
                   label + " for_each transform");
    requireNumbers(forEachResult, expected, label + " for_each result");
    require(input == original, label + " for_each changed the input");

    const std::string streamOutput = newPath("stream_result.txt");
    requireSuccess(sequence::outputResultStreams(streamOutput, containerResult,
                   error), error, label + " stream output");
    requireNumbers(readTextNumbers(streamOutput), expected,
                   label + " stream file values");
    const std::string algorithmOutput = newPath("algorithm_result.txt");
    requireSuccess(sequence::outputResultAlgorithms(algorithmOutput,
                   forEachResult, error), error, label + " algorithm output");
    requireNumbers(readTextNumbers(algorithmOutput), expected,
                   label + " algorithm file values");
}

template <typename Input, typename Output>
void checkLoadersAndErrors(const std::string& label) {
    std::string error;
    const std::string valid = newPath("valid_input.txt");
    writeFixture(valid, "2 5 -3 -4 0 7"); // A valid last token at EOF.
    const Input expected = {2, 5, -3, -4, 0, 7};
    Input streamInput;
    requireSuccess(sequence::loadDataStreams(valid, streamInput, error), error,
                   label + " stream load");
    require(streamInput == expected, label + " wrong stream input");
    Input algorithmInput;
    requireSuccess(sequence::loadDataAlgorithms(valid, algorithmInput, error),
                   error, label + " algorithm load");
    require(algorithmInput == expected, label + " wrong algorithm input");

    const std::string malformed[] = {
        "", " \n\t", "2 -3 x", "2 -3 4x", "2 1.5 -3", "2 -3 4.0",
        "2 -3 +", "2 -3 -", "2 -3 2147483648", "2 -3 -2147483649",
        "2 -3 999999999999999999999999999999", "51 -3", "-51 2"
    };
    for (std::size_t index = 0; index < sizeof(malformed) / sizeof(*malformed);
         ++index) {
        const std::string path = newPath("invalid_input.txt");
        writeFixture(path, malformed[index]);
        Input result;
        error.clear();
        requireFailure(sequence::loadDataStreams(path, result, error), error,
                       label + " malformed stream input " +
                       numberText(index));
        error.clear();
        requireFailure(sequence::loadDataAlgorithms(path, result, error), error,
                       label + " malformed algorithm input " +
                       numberText(index));
    }

    const Input noNegative = {2, 5, 0, 4};
    const Input empty;
    const Input* invalidInputs[] = {&noNegative, &empty};
    for (std::size_t index = 0; index < 2; ++index) {
        const Input original(*invalidInputs[index]);
        Output result = {123.5};
        error.clear();
        requireFailure(sequence::modifyContainer(*invalidInputs[index], result,
                       error), error, label + " invalid container transform");
        requireNumbers(result, {123.5}, label + " failed container result");
        error.clear();
        requireFailure(sequence::modifyForEach(*invalidInputs[index], result,
                       error), error, label + " invalid for_each transform");
        requireNumbers(result, {123.5}, label + " failed for_each result");
        error.clear();
        requireFailure(sequence::modifyIterators(invalidInputs[index]->begin(),
                       invalidInputs[index]->end(), std::back_inserter(result),
                       error), error, label + " invalid iterator transform");
        requireNumbers(result, {123.5}, label + " failed iterator result");
        require(*invalidInputs[index] == original,
                label + " failed transform changed input");
    }
    Output emptyResult;
    error.clear();
    requireFailure(sequence::appendStatistics(emptyResult, error), error,
                   label + " empty statistics");

    const std::string missing = newPath("missing_input.txt");
    Input result;
    error.clear();
    requireFailure(sequence::loadDataStreams(missing, result, error), error,
                   label + " missing stream input");
    error.clear();
    requireFailure(sequence::loadDataAlgorithms(missing, result, error), error,
                   label + " missing algorithm input");
}

void testSequenceContainers() {
    const std::vector<int> input = {2, 5, -3, -4, 0, 7};
    const std::vector<double> expected = {-6, 5, -3, 12, 0, 7, 33, 5.5};
    checkTransformation<std::vector<int>, std::vector<double> >(
        input, expected, "vector");
    checkTransformation<std::list<int>, std::list<double> >(
        input, expected, "list");
    checkTransformation<std::deque<int>, std::deque<double> >(
        input, expected, "deque");
    checkLoadersAndErrors<std::vector<int>, std::vector<double> >("vector");
    checkLoadersAndErrors<std::list<int>, std::list<double> >("list");
    checkLoadersAndErrors<std::deque<int>, std::deque<double> >("deque");
}

void testSequenceEdgeCases() {
    checkTransformation<std::vector<int>, std::vector<double> >(
        {2, -4, 6, -1}, {-8, 16, -24, -1, 49, 12.25}, "even first negative");
    checkTransformation<std::vector<int>, std::vector<double> >(
        {2, 5, 6, -4}, {-8, 5, -24, 16, 53, 13.25}, "late negative");
    checkTransformation<std::list<int>, std::deque<double> >(
        {-2, -4, 2}, {4, 8, -4, 16, 16.0 / 3.0}, "saved multiplier");
    checkTransformation<std::vector<int>, std::vector<double> >(
        {0, 2, 0, -1}, {0, -2, 0, -1, 3, 0.75}, "zeros");
    checkTransformation<std::vector<int>, std::vector<double> >(
        {-3}, {-3, 3, 3}, "one odd negative");
    checkTransformation<std::vector<int>, std::vector<double> >(
        {-4}, {16, 16, 16}, "one even negative");
    checkTransformation<std::vector<int>, std::vector<double> >(
        {-50, 50, -1, 0}, {2500, -2500, -1, 0, 5001, 1250.25},
        "input range boundaries");
}

void testSequenceFiles() {
    std::string error;
    const std::string generated[] = {newPath("generated_stream.txt"),
                                     newPath("generated_algorithm.txt")};
    requireSuccess(sequence::createDataStreams(generated[0], error), error,
                   "stream generation");
    requireSuccess(sequence::createDataAlgorithms(generated[1], error), error,
                   "algorithm generation");
    for (std::size_t index = 0; index < 2; ++index) {
        const std::vector<double> values = readTextNumbers(generated[index]);
        require(values.size() == 100, "generator must write 100 values");
        for (std::size_t item = 0; item < values.size(); ++item) {
            require(values[item] >= -50 && values[item] <= 50 &&
                    std::floor(values[item]) == values[item],
                    "generator produced a non-integer or out-of-range value");
        }
    }

    const std::string source = newPath("file_source.txt");
    const std::string sourceText = "2 5 -3 -4 0 7";
    writeFixture(source, sourceText);
    const std::string output = newPath("file_result.txt");
    requireSuccess(sequence::modifyFile(source, output, error), error,
                   "file transform");
    requireNumbers(readTextNumbers(output), {-6, 5, -3, 12, 0, 7, 33, 5.5},
                   "file transform result");
    require(readBytes(source) == sourceText, "file transform changed source");

    const std::string invalid = newPath("bad_file_source.txt");
    writeFixture(invalid, "2 -3 2147483648");
    error.clear();
    requireFailure(sequence::modifyFile(invalid, newPath("bad_result.txt"),
                   error), error, "file transform must reject overflow at EOF");
    require(readBytes(invalid) == "2 -3 2147483648",
            "failed file transform changed source");
    const std::string noNegative = newPath("no_negative.txt");
    writeFixture(noNegative, "0 2 5");
    error.clear();
    requireFailure(sequence::modifyFile(noNegative, newPath("none_result.txt"),
                   error), error, "file transform without a negative");
    require(readBytes(noNegative) == "0 2 5", "failed transform changed source");

    error.clear();
    requireFailure(sequence::modifyFile(source, source, error), error,
                   "same-path file transform");
    require(readBytes(source) == sourceText, "same-path transform clobbered source");
    const std::string alias = scratchDirectory + "/./" +
                              source.substr(source.find_last_of('/') + 1);
    error.clear();
    requireFailure(sequence::modifyFile(source, alias, error), error,
                   "equivalent-path file transform");
    require(readBytes(source) == sourceText, "alias transform clobbered source");

    const std::string sentinel = newPath("existing_sequence_output.txt");
    const std::string sentinelText = "existing output must survive\n";
    writeFixture(sentinel, sentinelText);
    const std::vector<double> values = {1.25, -2.5};
    error.clear();
    requireFailure(sequence::createDataStreams(sentinel, error), error,
                   "stream generation to existing output");
    require(readBytes(sentinel) == sentinelText, "generator overwrote output");
    error.clear();
    requireFailure(sequence::createDataAlgorithms(sentinel, error), error,
                   "algorithm generation to existing output");
    require(readBytes(sentinel) == sentinelText, "generator overwrote output");
    error.clear();
    requireFailure(sequence::outputResultStreams(sentinel, values, error), error,
                   "stream writer to existing output");
    require(readBytes(sentinel) == sentinelText, "stream writer overwrote output");
    error.clear();
    requireFailure(sequence::outputResultAlgorithms(sentinel, values, error),
                   error, "algorithm writer to existing output");
    require(readBytes(sentinel) == sentinelText, "algorithm writer overwrote output");
    error.clear();
    requireFailure(sequence::modifyFile(source, sentinel, error), error,
                   "file transform to existing output");
    require(readBytes(sentinel) == sentinelText, "transform overwrote output");
    require(readBytes(source) == sourceText, "rejected transform changed source");

    error.clear();
    requireFailure(sequence::modifyFile(newPath("missing.txt"),
                   newPath("missing_result.txt"), error), error,
                   "file transform with missing source");
    const std::string badDestination = newPath("missing_directory") + "/out.txt";
    error.clear();
    requireFailure(sequence::outputResultStreams(badDestination, values, error),
                   error, "stream writer to missing directory");
    error.clear();
    requireFailure(sequence::outputResultAlgorithms(badDestination, values,
                   error), error, "algorithm writer to missing directory");
}

personnel::Person makePerson(int id, int department, const char* name, char sex,
                             personnel::Date date, double accrued,
                             double withheld) {
    personnel::Person person = {};
    person.id = id;
    person.department = department;
    std::strncpy(person.fullName, name, sizeof(person.fullName) - 1);
    person.sex = sex;
    person.salary = id * 100.0;
    person.hired = date;
    person.bonusPercent = 10.0;
    person.incomeTax = id + 0.25;
    person.workedDays = 20;
    person.workingDays = 22;
    person.accrued = accrued;
    person.withheld = withheld;
    return person;
}

std::vector<personnel::Person> samplePeople() {
    return {
        makePerson(11, 7, "Ivan Petrov", 'M', {1, 2, 2020}, 100.25, 20.05),
        makePerson(22, 7, "Anna Sidorova", 'F', {2, 3, 2021}, 200.50, 30.10),
        makePerson(33, 9, "Maria Ivanova", 'F', {1, 2, 2020}, 300.75, 40.15)
    };
}

bool samePerson(const personnel::Person& left, const personnel::Person& right) {
    return left.id == right.id && left.department == right.department &&
           std::strcmp(left.fullName, right.fullName) == 0 &&
           left.sex == right.sex && near(left.salary, right.salary) &&
           left.hired.day == right.hired.day &&
           left.hired.month == right.hired.month &&
           left.hired.year == right.hired.year &&
           near(left.bonusPercent, right.bonusPercent) &&
           near(left.incomeTax, right.incomeTax) &&
           left.workedDays == right.workedDays &&
           left.workingDays == right.workingDays &&
           near(left.accrued, right.accrued) && near(left.withheld, right.withheld);
}

void requirePeople(std::vector<personnel::Person> actual,
                   std::vector<personnel::Person> expected,
                   const std::string& message) {
    require(actual.size() == expected.size(), message + ": wrong record count");
    const auto byId = [](const personnel::Person& left,
                         const personnel::Person& right) {
        return left.id < right.id;
    };
    std::sort(actual.begin(), actual.end(), byId);
    std::sort(expected.begin(), expected.end(), byId);
    for (std::size_t index = 0; index < actual.size(); ++index) {
        require(samePerson(actual[index], expected[index]),
                message + ": wrong record " + numberText(index));
    }
}

void requireDatabase(const std::string& path,
                     const std::vector<personnel::Person>& expected,
                     const std::string& message) {
    personnel::Database database;
    std::string error;
    requireSuccess(database.open(path, error), error, message + " open");
    std::vector<personnel::Person> actual;
    requireSuccess(database.readAll(actual, error), error, message + " read");
    requirePeople(actual, expected, message);
    for (std::size_t index = 0; index < expected.size(); ++index) {
        personnel::Person found = {};
        requireSuccess(database.get(expected[index].id, found, error), error,
                       message + " indexed lookup");
        require(samePerson(found, expected[index]), message + " wrong indexed record");
    }
}

void testPersonnelMutations() {
    std::string error;
    const std::string path = newPath("people.bin");
    std::vector<personnel::Person> people = samplePeople();
    personnel::Database database;
    require(!database.isOpen(), "new database must be closed");
    requireSuccess(database.create(path, people, error), error, "create database");
    require(database.isOpen() && database.path() == path,
            "created database must expose its open path");
    requireDatabase(path, people, "initial round trip");

    const personnel::Person fourth = makePerson(44, 9, "Petr Smirnov", 'M',
                                                {5, 5, 2022}, 400.0, 50.0);
    requireSuccess(database.append(fourth, error), error, "append record");
    people.push_back(fourth);
    requireDatabase(path, people, "append round trip");
    const std::string beforeDuplicate = readBytes(path);
    error.clear();
    requireFailure(database.append(people[0], error), error, "duplicate append");
    require(readBytes(path) == beforeDuplicate, "duplicate append changed database");

    personnel::Person firstReplacement = people.front();
    firstReplacement.id = 111;
    firstReplacement.department = 12;
    std::strcpy(firstReplacement.fullName, "First Replacement");
    firstReplacement.accrued = 111.75;
    requireSuccess(database.replace(11, firstReplacement, error), error,
                   "replace first record and its key");
    people.front() = firstReplacement;
    personnel::Person lastReplacement = people.back();
    lastReplacement.id = 444;
    std::strcpy(lastReplacement.fullName, "Last Replacement");
    requireSuccess(database.replace(44, lastReplacement, error), error,
                   "replace last record and its key");
    people.back() = lastReplacement;
    for (std::size_t index = 0; index < people.size(); ++index) {
        personnel::Person found = {};
        requireSuccess(database.get(people[index].id, found, error), error,
                       "current index after replacement");
        require(samePerson(found, people[index]), "stale index after replacement");
    }
    personnel::Person found = {};
    error.clear();
    requireFailure(database.get(11, found, error), error, "old first key");
    error.clear();
    requireFailure(database.get(44, found, error), error, "old last key");
    requireDatabase(path, people, "replacement persisted");

    const std::string beforeFailures = readBytes(path);
    error.clear();
    requireFailure(database.replace(111, people[1], error), error,
                   "replacement duplicate key");
    require(readBytes(path) == beforeFailures, "duplicate replacement changed data");
    error.clear();
    requireFailure(database.replace(9999, firstReplacement, error), error,
                   "replace missing key");
    require(readBytes(path) == beforeFailures, "missing replacement changed data");
    error.clear();
    requireFailure(database.erase(9999, error), error, "erase missing key");
    require(readBytes(path) == beforeFailures, "missing erase changed data");
    error.clear();
    requireFailure(database.get(9999, found, error), error, "get missing key");
    require(readBytes(path) == beforeFailures, "missing lookup changed data");

    requireSuccess(database.erase(111, error), error, "erase first record");
    people.erase(people.begin());
    requireSuccess(database.erase(444, error), error, "erase last record");
    people.pop_back();
    for (std::size_t index = 0; index < people.size(); ++index) {
        requireSuccess(database.get(people[index].id, found, error), error,
                       "current index after erasure");
        require(samePerson(found, people[index]), "stale index after erasure");
    }
    error.clear();
    requireFailure(database.get(111, found, error), error, "erased first key");
    error.clear();
    requireFailure(database.get(444, found, error), error, "erased last key");
    requireDatabase(path, people, "erasure persisted");
}

void testPersonnelFiltersAndReport() {
    std::string error;
    const std::string path = newPath("filter_source.bin");
    const std::vector<personnel::Person> people = samplePeople();
    personnel::Database database;
    requireSuccess(database.create(path, people, error), error, "filter source");
    const std::string original = readBytes(path);
    const std::string department = newPath("department.bin");
    requireSuccess(database.searchDepartment(7, department, error), error,
                   "department filter");
    requireDatabase(department, {people[0], people[1]}, "same-department matches");
    const std::string female = newPath("female.bin");
    requireSuccess(database.searchSex('F', female, error), error, "female filter");
    requireDatabase(female, {people[1], people[2]}, "female matches");
    const std::string male = newPath("male.bin");
    requireSuccess(database.searchSex('M', male, error), error, "male filter");
    requireDatabase(male, {people[0]}, "male matches");
    const std::string date = newPath("date.bin");
    requireSuccess(database.searchDate({1, 2, 2020}, date, error), error,
                   "date filter");
    requireDatabase(date, {people[0], people[2]}, "exact-date matches");
    const std::string empty = newPath("no_matches.bin");
    requireSuccess(database.searchDepartment(99, empty, error), error,
                   "filter without matches");
    requireDatabase(empty, {}, "empty filtered database");

    const std::string report = newPath("report.txt");
    requireSuccess(database.report(report, error), error, "personnel report");
    const std::string text = readBytes(report);
    requireReportTotal(text, "Количество сотрудников:", 3);
    requireReportTotal(text, "Итого начислено:", 601.50);
    requireReportTotal(text, "Итого удержано:", 90.30);
    requireReportTotal(text, "Итого к выплате:", 511.20);
    require(readBytes(path) == original, "filters or report changed the source");

    const std::string sentinel = newPath("existing_database_output.bin");
    std::string sentinelBytes = "do not replace existing output";
    sentinelBytes.push_back('\0');
    sentinelBytes += "binary";
    writeFixture(sentinel, sentinelBytes);
    personnel::Database other;
    error.clear();
    requireFailure(other.create(sentinel, people, error), error,
                   "create over existing output");
    require(readBytes(sentinel) == sentinelBytes, "create overwrote existing output");
    error.clear();
    requireFailure(database.searchDepartment(7, sentinel, error), error,
                   "department filter over existing output");
    require(readBytes(sentinel) == sentinelBytes, "department filter overwrote output");
    error.clear();
    requireFailure(database.searchSex('F', sentinel, error), error,
                   "sex filter over existing output");
    require(readBytes(sentinel) == sentinelBytes, "sex filter overwrote output");
    error.clear();
    requireFailure(database.searchDate({1, 2, 2020}, sentinel, error), error,
                   "date filter over existing output");
    require(readBytes(sentinel) == sentinelBytes, "date filter overwrote output");
    error.clear();
    requireFailure(database.report(sentinel, error), error,
                   "report over existing output");
    require(readBytes(sentinel) == sentinelBytes, "report overwrote output");

    error.clear();
    requireFailure(database.searchDepartment(7, path, error), error,
                   "filter over source database");
    require(readBytes(path) == original, "filter clobbered source database");
    error.clear();
    requireFailure(database.report(path, error), error, "report over source database");
    require(readBytes(path) == original, "report clobbered source database");
    const std::string alias = scratchDirectory + "/./" +
                              path.substr(path.find_last_of('/') + 1);
    error.clear();
    requireFailure(database.searchSex('F', alias, error), error,
                   "filter over equivalent source path");
    require(readBytes(path) == original, "alias filter clobbered source database");
}

void testPersonnelInvalidFiles() {
    std::string error;
    personnel::Database closed;
    std::vector<personnel::Person> actual;
    error.clear();
    requireFailure(closed.readAll(actual, error), error, "read closed database");
    error.clear();
    requireFailure(closed.open(newPath("missing_database.bin"), error), error,
                   "open missing database");

    const std::vector<personnel::Person> people = samplePeople();
    personnel::Database duplicate;
    error.clear();
    requireFailure(duplicate.create(newPath("duplicate.bin"),
                   {people[0], people[0]}, error), error, "create duplicate keys");
    const std::string valid = newPath("valid_for_partial.bin");
    personnel::Database source;
    requireSuccess(source.create(valid, people, error), error,
                   "create source for corrupt fixture");
    const std::string partial = newPath("partial_record.bin");
    writeFixture(partial, readBytes(valid) + '\0');
    personnel::Database corrupt;
    error.clear();
    requireFailure(corrupt.open(partial, error), error, "reject partial final record");
    require(!corrupt.isOpen(), "partial-record database must stay closed");

    const std::string empty = newPath("empty_database.bin");
    personnel::Database emptyDatabase;
    requireSuccess(emptyDatabase.create(empty, {}, error), error,
                   "create empty database");
    requireDatabase(empty, {}, "empty database round trip");
    requireSuccess(emptyDatabase.append(people[0], error), error,
                   "append to empty database");
    requireSuccess(emptyDatabase.erase(people[0].id, error), error,
                   "erase only record");
    requireDatabase(empty, {}, "empty database after erasing only record");
}

void testPersonnelRewriteCollisions() {
    const std::vector<personnel::Person> people = samplePeople();
    const char* suffixes[] = {".tmp", ".bak"};
    for (std::size_t index = 0; index < 2; ++index) {
        const std::string path = newPath("rewrite_source.bin");
        std::string error;
        personnel::Database database;
        requireSuccess(database.create(path, people, error), error,
                       "create rewrite collision source");
        const std::string original = readBytes(path);
        const std::string blocker = path + suffixes[index];
        const std::string sentinel = "an unrelated file must survive";
        writeFixture(blocker, sentinel);
        error.clear();
        requireFailure(database.erase(people[0].id, error), error,
                       "erase with occupied auxiliary file");
        require(readBytes(path) == original, "failed erase changed the source");
        require(readBytes(blocker) == sentinel, "erase replaced an auxiliary file");
        personnel::Person replacement = people[0];
        replacement.id = 111;
        error.clear();
        requireFailure(database.replace(people[0].id, replacement, error), error,
                       "replace with occupied auxiliary file");
        require(readBytes(path) == original, "failed replacement changed the source");
        require(readBytes(blocker) == sentinel,
                "replacement overwrote an auxiliary file");
        personnel::Person found = {};
        requireSuccess(database.get(people[0].id, found, error), error,
                       "index after rejected rewrite");
        require(samePerson(found, people[0]), "rejected rewrite changed the index");
        requireDatabase(path, people, "rejected rewrite round trip");
    }
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 2 || std::string(argv[1]).empty()) {
        std::cerr << "Usage: tests EXISTING_SCRATCH_DIRECTORY\n";
        return 2;
    }
    scratchDirectory = argv[1];
    struct TestCase {
        const char* name;
        void (*run)();
    };
    const TestCase tests[] = {
        {"sequence containers and malformed input", testSequenceContainers},
        {"sequence edge cases", testSequenceEdgeCases},
        {"sequence files and output protection", testSequenceFiles},
        {"personnel mutations and index", testPersonnelMutations},
        {"personnel filters, report and output protection", testPersonnelFiltersAndReport},
        {"personnel invalid and empty files", testPersonnelInvalidFiles},
        {"personnel rewrite collisions", testPersonnelRewriteCollisions}
    };
    int failures = 0;
    for (std::size_t index = 0; index < sizeof(tests) / sizeof(*tests); ++index) {
        try {
            tests[index].run();
            std::cout << "PASS " << tests[index].name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "FAIL " << tests[index].name << ": " << error.what() << '\n';
        }
    }
    std::cout << (failures == 0 ? "All regression tests passed.\n" :
                                  "Regression tests failed.\n");
    return failures == 0 ? 0 : 1;
}

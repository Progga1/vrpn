/** @file
	@brief Header

	@date 2011

	@author
	Ryan Pavlik
	<rpavlik@iastate.edu> and <abiryan@ryand.net>
	http://academic.cleardefinition.com/
	Iowa State University Virtual Reality Applications Center
	Human-Computer Interaction Graduate Program
*/

//          Copyright Iowa State University 2011.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)


#ifndef INCLUDED_Inspector_h_GUID_02c5132a_e54b_4e32_a8ce_d9bf088b0d47
#define INCLUDED_Inspector_h_GUID_02c5132a_e54b_4e32_a8ce_d9bf088b0d47

// Internal Includes
// - none

// Library/third-party includes
#include <QObject>
#include <QByteArray>

// Standard includes
// - none

/// Class to extract bytes from an array and turn into some kind of value
class Inspector : public QObject {
		Q_OBJECT
	public:
		explicit Inspector(std::size_t first_index, std::size_t length, bool signedVal, bool bigEndian = false, QObject * parent = NULL);
		~Inspector() {}

	signals:
		void newValue(float val);
		void newValue(float elapsed, float val);
	public slots:
		void updatedData(QByteArray buf, qint64 timestamp);

	private:
		void _sendNewValue(qint64 timestamp, float val);
		std::size_t _first;
		std::size_t _length;
		bool _signed;
		bool _bigEndian;
};

#endif // INCLUDED_Inspector_h_GUID_02c5132a_e54b_4e32_a8ce_d9bf088b0d47

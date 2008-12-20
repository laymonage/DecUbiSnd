/*
 SegmentsList.h : A list of segments
*/

#pragma once

namespace NDecFunc
{
	class CSegment;

	class CSegmentsList : protected std::vector<CSegment*>
	{
	protected:
		std::string m_Filename;

	public:
		CSegmentsList();
		virtual ~CSegmentsList();

		virtual void SetFilename(std::string Filename);
		virtual std::string GetFilename() const;
		virtual void ScanFile();
		virtual CSegment* CreateSegment(std::streamoff Offset, std::streamsize Size) const;

		virtual void Add(CSegment* Segment);
		virtual unsigned long Insert(CSegment* Segment);
		virtual void Remove(unsigned long Index);
		virtual void Remove(const std::vector<unsigned long>& Indicies);
		virtual void Duplicate(unsigned long Index);
		virtual void Duplicate(const std::vector<unsigned long>& Indicies);
		virtual bool CanMix(const std::vector<unsigned long>& Indicies) const;
		virtual bool CanUnmix(const std::vector<unsigned long>& Indicies) const;
		virtual void Mix(const std::vector<unsigned long>& Indicies);
		virtual void Unmix(const std::vector<unsigned long>& Indicies);
		virtual CSegment* Get(unsigned long Index);
		virtual bool IsValid(unsigned long Index) const;
		virtual unsigned long GetCount() const;
		virtual void Clear();
	};
};

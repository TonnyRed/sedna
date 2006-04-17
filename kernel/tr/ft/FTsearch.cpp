#include "FTsearch.h"
#include "tr_globals.h"
using namespace dtSearch;

/////////////////////////////////////////////////////////////
//
//  DSearchJob



class GenericDataSource {
    public:
        GenericDataSource();
        void makeInterface(dtsDataSource& dest);
        virtual ~GenericDataSource();
        virtual int getNextDoc(dtsInputStream& dest) = 0;
        virtual int rewind() = 0;
        static int getNextDocCB(void *pData, dtsInputStream& dest);
        static int rewindCB(void *pData);
        static GenericDataSource *safeCast(void *pData);
        void makeInterface(dtsInputStream& dest);
   };

//
//  A TextInputStream is a "document" to be indexed.  
//  The SampleDataSource object will create a TextInputStream
//  for each document it wants to supply to the dtSearch Engine.
//  The dtSearch Engine will use the function pointers in a
//  dtsInputStream to access a TextInputStream.
//


const long TextInputStreamID = 0x01010101;


//
//  SampleDataSource is a data source based on a table of
//  records (below), each of which has an associated 
//  line of text.
//

class SampleDataSource : public GenericDataSource {
    public:
        SampleDataSource(xptr _node_);
        virtual int getNextDoc(dtsInputStream& s);
        virtual int rewind();
        static void recordToFilename(char *dest,xptr _node_);       
    protected:
        xptr node;
		int pos;
		t_str_buf in_buf;
    };
//
//  SampleDataSource is a data source based on a table of
//  records (below), each of which has an associated 
//  line of text.
//



GenericDataSource::GenericDataSource()   
{
}

GenericDataSource::~GenericDataSource()
{ 
}

int GenericDataSource::rewindCB(void *pData)
{   GenericDataSource *s = GenericDataSource::safeCast(pData);
    if (s)
        return s->rewind();
    else
        return FAIL;
}

int GenericDataSource::getNextDocCB(void *pData, dtsInputStream& dest)
{   GenericDataSource *s = GenericDataSource::safeCast(pData);
    if (s)
        return s->getNextDoc(dest);
    else
        return FAIL;
}

GenericDataSource *GenericDataSource::safeCast(void *pData)
{   GenericDataSource *s = (GenericDataSource *) pData;
    if (s)
        return s;
    return NULL;
}

void GenericDataSource::makeInterface(dtsDataSource& dest)
{   memset(&dest, 0, sizeof dest);
    dest.pData = this;
    dest.rewind = GenericDataSource::rewindCB;
    dest.getNextDoc = GenericDataSource::getNextDocCB;
}

SednaTextInputStream::SednaTextInputStream(dtsFileInfo* info,ft_index_type _cm_,pers_sset<ft_custom_cell,unsigned short>* _custom_tree_) :
    cm(_cm_),
    custom_tree(_custom_tree_),
    idTextInputStream(TextInputStreamID),
	fileInfo(info)
	{   
	 
    
}

SednaTextInputStream::~SednaTextInputStream()
{   
}

int SednaTextInputStream::read_mem(void *dest, long bytes)
{
	if (bytes + pos >= in_buf.get_size())
		bytes = in_buf.get_size() - pos;
    if (bytes > 0)
		memmove(dest, (char*)in_buf.get_ptr_to_text() + pos, bytes);
    pos += bytes;
    return bytes;
}

void SednaTextInputStream::seek_mem(long where)
{   
	pos = where;
}

int SednaTextInputStream::read_estr(void *dest, long bytes)
{
	if (bytes + pos > in_buf.get_size())
        bytes = in_buf.get_size() - pos;
	long bytes_left = bytes;
    while (bytes_left-- > 0)
	{
		*(char*)dest = *estr_it;
		dest = (char*)dest + 1;
		++estr_it;
	}
    pos += bytes;
    return bytes;
}

void SednaTextInputStream::seek_estr(long where)
{   
	if (pos > where)
		estr_it -= (pos - where);
	else
		estr_it += (where - pos);
	pos = where;
}

long SednaTextInputStream::readCBmem(void *pData, void *dest, long bytes)
{   SednaTextInputStream *s = SednaTextInputStream::safeCast(pData);
    if (s)
        return s->read_mem(dest, bytes);
    else
        return FAIL;
}

void SednaTextInputStream::seekCBmem(void *pData, long where)
{   SednaTextInputStream *s = SednaTextInputStream::safeCast(pData);
    if (s)
        s->seek_mem(where);
}
 
long SednaTextInputStream::readCBestr(void *pData, void *dest, long bytes)
{   SednaTextInputStream *s = SednaTextInputStream::safeCast(pData);
    if (s)
        return s->read_estr(dest, bytes);
    else
        return FAIL;
}

void SednaTextInputStream::seekCBestr(void *pData, long where)
{   SednaTextInputStream *s = SednaTextInputStream::safeCast(pData);
    if (s)
        s->seek_estr(where);
}
 
void SednaTextInputStream::releaseCB(void *pData)
{   /*SednaTextInputStream *s = safeCast(pData);
    if (s)
        delete s;*/
}

SednaTextInputStream *SednaTextInputStream::safeCast(void *pData)
{   SednaTextInputStream *s = (SednaTextInputStream *) pData;
    if (s && (s->idTextInputStream == TextInputStreamID))
        return s;
    return NULL;
}   

void SednaTextInputStream::makeInterface(dtsInputStream& dest,xptr& node)
{   
	in_buf.clear();
	dest.pData = this;
    dest.filename = fileInfo->filename;
	dest.typeId = it_XML;
	CHECKP(node);
	print_node_to_buffer(node,in_buf,cm,custom_tree);
	pos = 0;
	dest.size = in_buf.get_size();
	fileInfo->size=dest.size;
	if (in_buf.get_type() == text_mem)
	{
	    dest.read = SednaTextInputStream::readCBmem;
		dest.seek = SednaTextInputStream::seekCBmem;
	}
	else
	{
		estr_it = e_string_iterator_first(dest.size, *(xptr*)in_buf.get_ptr_to_text());
	    dest.read = SednaTextInputStream::readCBestr;
		dest.seek = SednaTextInputStream::seekCBestr;
	}
    dest.release = SednaTextInputStream::releaseCB;
    dest.modified.copy(fileInfo->modified);
    dest.created.copy(fileInfo->created);
}

SednaDataSource::SednaDataSource(ft_index_type _cm_,pers_sset<ft_custom_cell,unsigned short>* _custom_tree_):
cm(_cm_),custom_tree(_custom_tree_)
{
	tis = new SednaTextInputStream(&fileInfo, cm ,custom_tree);
	
}
void SednaDataSource::recordToFilename(char *dest,xptr _node_)
{    
	sprintf(dest, "%d-0x%x.xml", (int)(_node_.layer),(unsigned int)(_node_.addr));
}
xptr SednaDataSource::filenameToRecord(const char *dest)
{   
	int layer;
	unsigned int addr;
	const char *fn = dest, *t = dest;
	while (*t)
	{
		if (*t == '/' || *t == '\\')
			fn = t+1;
		t++;
	}
	sscanf(fn, "%d-0x%x.xml", &layer, &addr);
	return removeIndirection(xptr(layer, (void*)addr));
}

int SednaDataSource::getNextDoc(dtsInputStream& dest)
{   
	/*tuple t(1);
	seq->op->next(t);
		//Preliminary node analysis
   if (t.is_eos()) 
		return FAIL;
   tuple_cell& tc= t.cells[0];
	if (!tc.is_node())
	{
			throw USER_EXCEPTION(SE2031);
	}
    xptr node=tc.get_node();
	crmstringostream st;
	print_node_indent(node, st,xml);
	st<<'\x0';
	char* f=st.str();
    //in_buf.set(dm_string_value(node));
    dtsFileInfo fileInfo;
    recordToFilename(fileInfo.filename,node);
	fileInfo.size = strlen(f);
    fileInfo.modified.year = 1996;
    fileInfo.modified.month = 1;
    fileInfo.modified.day = 1;
	TextInputStream *s = new TextInputStream(fileInfo, f);
    s->makeInterface(dest);
     */
	xptr node=get_next_doc();
	if (node==XNULL) return FAIL;
	CHECKP(node);
	recordToFilename(fileInfo.filename,((n_dsc*)XADDR(node))->indir);
//	fileInfo.size = strlen(f);
    fileInfo.modified.year = 1996;
    fileInfo.modified.month = 1;
    fileInfo.modified.day = 1;	
    tis->makeInterface(dest,node);
	return SUCCESS;
}

int SednaDataSource::rewind()
{   //pos = 0;
    //if (pos) seq->op->reopen();
	return SUCCESS;    
}
CreationSednaDataSource::CreationSednaDataSource(ft_index_type _cm_,pers_sset<ft_custom_cell,unsigned short>* _custom_tree_,std::vector<xptr>* _first_nodes_):SednaDataSource(_cm_,_custom_tree_),first_nodes(_first_nodes_),tmp(XNULL)
{
	it=first_nodes->begin();
}
xptr CreationSednaDataSource::get_next_doc()
{
	if (tmp==XNULL) 
	{
		if (it!=first_nodes->end())
			tmp=*it;
		return tmp;
	}
	tmp=getNextDescriptorOfSameSortXptr(tmp);
	if (tmp==XNULL)
	{
		if (++it==first_nodes->end())return XNULL;
		tmp=*it;		
	}
	return tmp;	
}
int CreationSednaDataSource::rewind()
{
	it=first_nodes->begin();
	tmp=XNULL;
	return SUCCESS;
}
UpdateSednaDataSource::UpdateSednaDataSource(ft_index_type _cm_,pers_sset<ft_custom_cell,unsigned short>* _custom_tree_,xptr_sequence * _seq_):SednaDataSource(_cm_,_custom_tree_),seq(_seq_)
{
	it=seq->begin();
}
xptr UpdateSednaDataSource::get_next_doc()
{
	while (it!=seq->end())
	{
		xptr ptr = removeIndirection(*it++);
		if (ptr != XNULL)
			return ptr;
	}
	return XNULL;
}
int UpdateSednaDataSource::rewind()
{
	it=seq->begin();
	return SUCCESS;
}
OperationSednaDataSource::OperationSednaDataSource(ft_index_type _cm_,pers_sset<ft_custom_cell,unsigned short>* _custom_tree_,PPOpIn* _op_):SednaDataSource(_cm_,_custom_tree_),op(_op_),t(1)
{
}
xptr OperationSednaDataSource::get_next_doc()
{
   op->op->next(t);
		//Preliminary node analysis
   if (t.is_eos()) 
		return XNULL;
   tuple_cell& tc= t.cells[0];
   if (!tc.is_node())
   {
	   throw USER_EXCEPTION(SE2031);
   }
   return tc.get_node();
}
int OperationSednaDataSource::rewind()
{
	op->op->reopen();
	return SUCCESS;
}
void SednaSearchJob::OnError(long errorCode, const char *msg)
{
}
void SednaSearchJob::OnFound(long totalFiles,
                 long totalHits, const char *name, long hitsInFile, dtsSearchResultsItem& item)
{
	DSearchJob::OnFound(totalFiles, totalHits, name, hitsInFile, item);
	DSearchJob::VetoThisItem();
	res = SednaDataSource::filenameToRecord(name);
	if (hilight)
		hl->convert_node(SednaDataSource::filenameToRecord(name),item.hits,item.hitCount);
	UUnnamedSemaphoreUp(&sem1);
	UUnnamedSemaphoreDown(&sem2);
}
SednaSearchJob::SednaSearchJob(PPOpIn* _seq_,ft_index_type _cm_,pers_sset<ft_custom_cell,unsigned short>* _custom_tree_,bool _hilight_, bool _hl_fragment_):seq(_seq_), hilight(_hilight_), hl_fragment(_hl_fragment_)
{
	AttachDataSource(new OperationSednaDataSource(_cm_,_custom_tree_,_seq_),true);
	dtth=NULL;
	this->SuppressMessagePump();
	if (hilight)
	{
		if (_cm_ == ft_xml_hl)
			hl=new SednaConvertJob(ft_xml,_custom_tree_, hl_fragment);	
		else
			hl=new SednaConvertJob(_cm_,_custom_tree_, hl_fragment);
	}
}
SednaSearchJob::SednaSearchJob(bool _hilight_, bool _hl_fragment_):seq(NULL),hilight(_hilight_),hl_fragment(_hl_fragment_)
{
	dtth=NULL;
	this->SuppressMessagePump();
}
void SednaSearchJob::set_request(tuple_cell& request)
{
	this->Request.setU8(t_str_buf(request).c_str());
}
void SednaSearchJob::get_next_result(tuple &t)
{
	if (dtth==NULL)
	{
		UUnnamedSemaphoreCreate(&sem1, 0, NULL);
		UUnnamedSemaphoreCreate(&sem2, 0, NULL);
        uCreateThread(
        ThreadFunc,                  // thread function 
        this,						 // argument to thread function 
        &dtth,                       // use default creation flags 
        0, NULL);
	}
	else
		UUnnamedSemaphoreUp(&sem2);    
	UUnnamedSemaphoreDown(&sem1);
    if (res==XNULL)
	{
		/*
		dtsOptions opts;
		short result;
		dtssGetOptions(opts, result);
		opts.fieldFlags = save_field_flags;
		dtssSetOptions(opts, result);*/
		t.set_eos();
		dtth = NULL;
		UUnnamedSemaphoreRelease(&sem1);
		UUnnamedSemaphoreRelease(&sem2);
	}
	else
	{
		if (!hilight)
			t.copy(tuple_cell::node(res));
		else
			t.copy(SednaConvertJob::result.content());
		
	}
}
void SednaSearchJob::set_index(tuple_cell& name)
{
	ft_index_cell* ft_idx=ft_index_cell::find_index(t_str_buf(name).c_str());
	if (ft_idx==NULL)
		throw USER_EXCEPTION(SE1061);
	std::string index_path1 = std::string(SEDNA_DATA) + std::string("\\data\\")
		+ std::string(db_name) + std::string("_files\\dtsearch\\");
	std::string index_path = index_path1 + std::string(ft_idx->index_title);
	this->AddIndexToSearch(index_path.c_str());
	if (hilight)
		hl=new SednaConvertJob(ft_idx->ftype,ft_idx->custom_tree, hl_fragment);
	
}

void SednaSearchJob::reopen()
{
	seq->op->reopen();
	dtth=NULL;
}
SednaSearchJob::~SednaSearchJob()
{
}
DWORD WINAPI SednaSearchJob::ThreadFunc( void* lpParam )
{
	
	//if (((SednaSearchJob*)lpParam)->hilight)
	{
		
		dtsOptions opts;
		short result;
		dtssGetOptions(opts, result);
		((SednaSearchJob*)lpParam)->save_field_flags = opts.fieldFlags;
		//opts.fieldFlags |= dtsoFfXmlSkipAttributes  | dtsoFfXmlHideFieldNames | dtsoFfSkipFilenameField;
		opts.fieldFlags = dtsoFfXmlHideFieldNames | dtsoFfSkipFilenameField | dtsoFfXmlSkipAttributes;
		dtssSetOptions(opts, result);
	}
	((SednaSearchJob*)lpParam)->Execute();
	if (((SednaSearchJob*)lpParam)->GetErrorCount()>0)
	{
		const dtsErrorInfo * ptr= ((SednaSearchJob*)lpParam)->GetErrors();
		/*for (int i=0;i<ptr->getCount();i++)
			std::cout<<ptr->getMessage(i);*/
	}

	((SednaSearchJob*)lpParam)->res = XNULL;
	UUnnamedSemaphoreUp(&(((SednaSearchJob*)lpParam)->sem1));
	return 0;
}
void SednaSearchJob::OnSearchingIndex(const char * indexPath)
{
	//std::cout<<"Searching: "<<indexPath;
}


template <class Iterator>
SednaStringHighlighter<Iterator>::SednaStringHighlighter(const Iterator &_str_it_, 
														 const Iterator &_str_end_, 
														 long* _ht_,
														 long _ht_cnt_, 
														 bool _hl_fragment_, 
														 e_str_buf *_result_) :
									str_it(_str_it_),
									str_end(_str_end_),
									ht(_ht_),
									ht_cnt(_ht_cnt_),
									hl_fragment(_hl_fragment_),
									result(_result_)

{
	result->reinit();
	qsort(ht, ht_cnt, sizeof(long), cmp_long);
}


template <typename Iterator>
void SednaStringHighlighter<Iterator>::run()
{
	current_word = 0;
	current_word_tok = 0;
	current_ht_idx = 0;
	tag_l = 0;
	tag_w = 0;

	if (hl_fragment)
	{
		ht0 = -1;
		Iterator str_it_save = str_it;
		Iterator str_end_save = str_end;
		//first parse_doc pass: compute ht0
		cur_ch = getch(str_it, str_end);
		parse_doc();

		//second pass: get the fragment
		str_it = str_it_save;
		str_end = str_end_save;
		current_word = 0;
		current_word_tok = 0;
		current_ht_idx = 0;
		tag_l = 0;
		tag_w = 0;

		cur_ch = getch(str_it, str_end);
		parse_doc();

		if (fragment_pref_ch == 0)
			cur_ch = getch(fragment_start, fragment_end);
		else
			cur_ch = fragment_pref_ch;
		current_word_tok = fragment_start_word_tok_num;
		current_ht_idx = 0; //FIXME?
		if (fragment_start_split)
			result->append_mstr("...");
		copy_doc(fragment_start, fragment_end);
		if (fragment_end_split)
			result->append_mstr("...");
	}
	else
	{
		cur_ch = getch(str_it, str_end);
		copy_doc(str_it, str_end);
	}
}

SednaConvertJob::SednaConvertJob(ft_index_type _cm_,pers_sset<ft_custom_cell,unsigned short>* _custom_tree_, bool _hl_fragment_) :
						cm(_cm_), custom_tree(_custom_tree_), hl_fragment(_hl_fragment_)
	{
}

template <class Iterator>
int SednaStringHighlighter<Iterator>::getch(Iterator &str_it, Iterator &str_end)
{
	unsigned char ch=*(str_it);
	int r;

	if (str_it >= str_end)
		return -1;

	//FIXME - check str_end

	++str_it;
	if (ch < 128)
	{
			r = ch;
			last_ch_len = 1;
	}
	else if (ch < 224) //FIXME: ch mustbe >= 192
	{
		r = ch - 192; r <<= 6;
		ch = *(str_it); r += ch - 128; ++str_it;
		last_ch_len = 2;
	}
	else if (ch < 240)
	{
		r = ch - 224; r <<= 6;
		ch = *(str_it); r += ch - 128; r <<= 6; ++str_it;
		ch = *(str_it); r += ch - 128; ++str_it;
		last_ch_len = 3;
	}
	else if (ch < 248)
	{
		r = ch - 240; r <<= 6;
		ch = *(str_it); r += ch - 128; r <<= 6; ++str_it;
		ch = *(str_it); r += ch - 128; r <<= 6; ++str_it;
		ch = *(str_it); r += ch - 128; ++str_it;
		last_ch_len = 4;
	}
	else if (ch < 252)
	{
		r = ch - 248; r <<= 6;
		ch = *(str_it); r += ch - 128; r <<= 6; ++str_it;
		ch = *(str_it); r += ch - 128; r <<= 6; ++str_it;
		ch = *(str_it); r += ch - 128; r <<= 6; ++str_it;
		ch = *(str_it); r += ch - 128; ++str_it;
		last_ch_len = 5;
	}
	else // ch mustbe < 254
	{
		r = ch - 252; r <<= 6;
		ch = *(str_it); r += ch - 128; r <<= 6; ++str_it;
		ch = *(str_it); r += ch - 128; r <<= 6; ++str_it;
		ch = *(str_it); r += ch - 128; r <<= 6; ++str_it;
		ch = *(str_it); r += ch - 128; r <<= 6; ++str_it;
		ch = *(str_it); r += ch - 128; ++str_it;
		last_ch_len = 6;
	}
	return r;
}

template <class Iterator>
void SednaStringHighlighter<Iterator>::putch(const int ch)
{
	char x;
	if (ch < (1 << 7)) {
		x = ch; result->append_mstr(&x, 1);
    } else if (ch < (1 << 11)) {
		x = ((ch >> 6) | 0xc0); result->append_mstr(&x, 1);
		x = ((ch & 0x3f) | 0x80); result->append_mstr(&x, 1);
    } else if (ch < (1 << 16)) {
		x = ((ch >> 12) | 0xe0); result->append_mstr(&x, 1);
		x = (((ch >> 6) & 0x3f) | 0x80); result->append_mstr(&x, 1);
		x = ((ch & 0x3f) | 0x80); result->append_mstr(&x, 1);
    } else if (ch < (1 << 21)) {
		x = ((ch >> 18) | 0xf0); result->append_mstr(&x, 1);
		x = (((ch >> 12) & 0x3f) | 0x80); result->append_mstr(&x, 1);
		x = (((ch >> 6) & 0x3f) | 0x80); result->append_mstr(&x, 1);
		x = ((ch & 0x3f) | 0x80); result->append_mstr(&x, 1);
    }
}



static inline bool is_tag_char(int ch)
{
	return ((!iswpunct(ch) && !iswspace(ch) && ch != '<' && ch != SednaStringHighlighter<char*>::EOF_ch) || ch == '-');
}

template <class Iterator>
void SednaStringHighlighter<Iterator>::copy_tag(Iterator &str_it, Iterator &str_end, bool copy)
{
	//TODO: implement

	U_ASSERT(cur_ch == '<');
	if (copy && !hl_fragment)
		putch(cur_ch);
	cur_ch = getch(str_it, str_end);
	if (cur_ch == '/')
	{
		if (copy && !hl_fragment)
			putch(cur_ch);
		cur_ch = getch(str_it, str_end);
		
		tag_l--;
		if (tag_l == 1)
		{
			current_word_tok += tag_w;
			while (current_ht_idx < ht_cnt && ht[current_ht_idx] <= current_word_tok)
					current_ht_idx++;
			tag_w = 0;
		}
	}
	else
	{
		tag_l++;
//		tag_w++;
		/*
		current_word_tok ++;
		while (current_ht_idx < ht_cnt && ht[current_ht_idx] <= current_word_tok)
				current_ht_idx++;*/
	}


	if (is_tag_char(cur_ch))
	{
		while (is_tag_char(cur_ch))
		{
			if (copy && !hl_fragment)
				putch(cur_ch);
			cur_ch = getch(str_it, str_end);
		}
	}
	do
	{
		if (!iswpunct(cur_ch) && !iswspace(cur_ch) && cur_ch != '<' && cur_ch != EOF_ch)
		{
			while (!iswpunct(cur_ch) && !iswspace(cur_ch) && cur_ch != '<' && cur_ch != EOF_ch)
			{
				if (copy && !hl_fragment)
					putch(cur_ch);
				cur_ch = getch(str_it, str_end);
			}
			
			while (iswspace(cur_ch) && cur_ch != EOF_ch)
			{
				if (copy && !hl_fragment)
					putch(cur_ch);
				cur_ch = getch(str_it, str_end);
			}
			//if (cur_ch == '=')
			{
//		tag_w++;
				/*
		current_word_tok ++;
		while (current_ht_idx < ht_cnt && ht[current_ht_idx] <= current_word_tok)
				current_ht_idx++;*/
			}
		}
		else if (cur_ch == '/')
		{
			if (copy && !hl_fragment)
				putch(cur_ch);
			cur_ch = getch(str_it, str_end);
			if (cur_ch == '>')
			{
				tag_l--;//FIXME
				break;
			}
		}
		else
		{
			if (cur_ch == EOF_ch || cur_ch == '>')
				break;
			if (copy && !hl_fragment)
				putch(cur_ch);
			cur_ch = getch(str_it, str_end);
		}
	} while (true);
	if (copy && !hl_fragment)
		putch(cur_ch);
	cur_ch = getch(str_it, str_end);
}

template <class Iterator>
void SednaStringHighlighter<Iterator>::parse_tag()
{
	copy_tag(str_it, str_end, false);
}

template <class Iterator>
void SednaStringHighlighter<Iterator>::copy_doc(Iterator &str_it, Iterator &str_end)
{
	while (cur_ch != EOF_ch)
	{
		if (cur_ch == (int)'<')
		{
			copy_tag(str_it, str_end);
		}
		else if (iswpunct(cur_ch) || iswspace(cur_ch))
		{
			putch(cur_ch);
			cur_ch = getch(str_it, str_end);
		}
		else
		{
			bool hl_word = false;
			current_word++;
			current_word_tok++;
			if (current_ht_idx < ht_cnt && ht[current_ht_idx] == current_word_tok)
			{
				hl_word = true;
				current_ht_idx++;
			}
			if (hl_word)
				result->append_mstr("<hit>");
			while (!iswpunct(cur_ch) && !iswspace(cur_ch) && cur_ch != '<' && cur_ch != EOF_ch)
			{
				putch(cur_ch);
				cur_ch = getch(str_it, str_end);
			}
			if (hl_word)
				result->append_mstr("</hit>");
		}
	}
}


template <class Iterator>
void SednaStringHighlighter<Iterator>::parse_doc()
{
	fragment_pref_ch = 0;
	fragment_start = str_it;
	fragment_start_split = false;
	U_ASSERT(current_word_tok == 0);
	U_ASSERT(current_word == 0);
	fragment_start_word_tok_num = -1;
	fragment_start_word_num = -1;
	last_eff_ch = 0;

	while (cur_ch != EOF_ch)
	{
		if (cur_ch == (int)'<')
		{
			parse_tag();
		}
		else if (iswpunct(cur_ch) || iswspace(cur_ch))
		{
			if (!iswspace(cur_ch))
				last_eff_ch = cur_ch;
			cur_ch = getch(str_it, str_end);
		}
		else
		{
			bool hl_word = false;
			current_word++;
			current_word_tok++;
			if (current_ht_idx < ht_cnt && ht[current_ht_idx] == current_word_tok)
			{
				if (ht0 == -1)
				{
					ht0 = current_word;
					//FIXME: return should return through recursive calls (but there are none of these here, for now)
					return; //end of the first pass
				}
				current_ht_idx++;
			}
			if (ht0 != -1) //FIXME?
			{
			if ((last_eff_ch == '.' && iswupper(cur_ch)) || last_eff_ch == 0)
			{ //sentence start
				if (current_word <= ht0 - min_words_before || fragment_start_word_num == -1)
				{
					fragment_start = str_it;
					fragment_start_word_tok_num = current_word_tok-1;
					fragment_start_word_num = current_word-1;
					fragment_pref_ch = cur_ch;
					fragment_start_split = false;
				}
				else
				{
					if (current_word >= ht0 + min_words_after)
					{
						//cut here
						fragment_end = str_it;
						fragment_end -= last_ch_len;
						fragment_end_split = false;
						//FIXME: return should return through recursive calls (but there are none of these here, for now)
						return;
					}
				}
			}
			else
			{
				if (current_word <= ht0)
				{
					if (current_word == ht0 - max_words_before) //FIXME
					{
						fragment_start = str_it;
						fragment_start_word_tok_num = current_word_tok-1;
						fragment_start_word_num = current_word-1;
						fragment_pref_ch = cur_ch;
						fragment_start_split = true;
					}
				}
				else
				{
					if (current_word == ht0 + max_words_after) //FIXME
					{
						//cut here
						fragment_end = str_it;
						fragment_end -= last_ch_len;
						fragment_end_split = true;
						//FIXME: return should return through recursive calls (but there are none of these here, for now)
						return;
					}
				}
			}
			}
			last_eff_ch = cur_ch; //no need to update it in the loop, we only need to know it's not punctuation
			while (!iswpunct(cur_ch) && !iswspace(cur_ch) && cur_ch != '<' && cur_ch != EOF_ch)
				cur_ch = getch(str_it, str_end);
		}
	}
	fragment_end = str_it;
	fragment_end_split = false;
}

static int cmp_long(const void *a, const void *b)
{
	return int(*((long*)a) - *((long*)b));
}

void SednaConvertJob::convert_node(xptr &node,long* _ht_,long _ht_cnt_)
{
	in_buf.clear();
	CHECKP(node);
	print_node_to_buffer(node,in_buf,cm,custom_tree);
	if (in_buf.get_type() == text_mem)
	{
		char *str_it = (char*)in_buf.get_ptr_to_text();
		char *str_it_end = str_it + in_buf.get_size();
		SednaStringHighlighter<char *> hl(str_it, str_it_end, _ht_, _ht_cnt_, hl_fragment, &result);
		hl.run();
	}
	else
	{
		e_string_iterator_first estr_it(in_buf.get_size(), *(xptr*)in_buf.get_ptr_to_text());
		e_string_iterator_first estr_it_end(0, *(xptr*)in_buf.get_ptr_to_text());
		SednaStringHighlighter<e_string_iterator> hl(estr_it, estr_it_end, _ht_, _ht_cnt_, hl_fragment, &result);
		hl.run();
	}

}
void SednaConvertJob::OnOutput(const char * txt, int length)
{
	result.append_mstr(txt,length);
}

e_str_buf SednaConvertJob::result;
t_str_buf SednaConvertJob::in_buf;
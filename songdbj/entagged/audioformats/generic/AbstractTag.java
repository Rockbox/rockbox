/*
 *  ********************************************************************   **
 *  Copyright notice                                                       **
 *  **																	   **
 *  (c) 2003 Entagged Developpement Team				                   **
 *  http://www.sourceforge.net/projects/entagged                           **
 *  **																	   **
 *  All rights reserved                                                    **
 *  **																	   **
 *  This script is part of the Entagged project. The Entagged 			   **
 *  project is free software; you can redistribute it and/or modify        **
 *  it under the terms of the GNU General Public License as published by   **
 *  the Free Software Foundation; either version 2 of the License, or      **
 *  (at your option) any later version.                                    **
 *  **																	   **
 *  The GNU General Public License can be found at                         **
 *  http://www.gnu.org/copyleft/gpl.html.                                  **
 *  **																	   **
 *  This copyright notice MUST APPEAR in all copies of the file!           **
 *  ********************************************************************
 */
package entagged.audioformats.generic;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import entagged.audioformats.Tag;

public abstract class AbstractTag implements Tag {
	
	
	protected HashMap fields = new HashMap();
	protected int commonNumber = 0;
	
	public List getTitle() {
        return get(getTitleId());
    }
    public List getAlbum() {
        return get(getAlbumId());
    }
    public List getArtist() {
        return get(getArtistId());
    }
    public List getGenre() {
        return get(getGenreId());
    }
    public List getTrack() {
        return get(getTrackId());
    }
    public List getYear() {
        return get(getYearId());
    }
    public List getComment() {
        return get(getCommentId());
    }
    
    public String getFirstTitle() {
        List l = get(getTitleId());
        return (l.size() != 0) ? ((TagTextField)l.get(0)).getContent() : "";
    }
    public String getFirstAlbum() {
        List l =  get(getAlbumId());
        return (l.size() != 0) ? ((TagTextField)l.get(0)).getContent() : "";
    }
    public String getFirstArtist() {
        List l =  get(getArtistId());
        return (l.size() != 0) ? ((TagTextField)l.get(0)).getContent() : "";
    }
    public String getFirstGenre() {
        List l =  get(getGenreId());
        return (l.size() != 0) ? ((TagTextField)l.get(0)).getContent() : "";
    }
    public String getFirstTrack() {
        List l =  get(getTrackId());
        return (l.size() != 0) ? ((TagTextField)l.get(0)).getContent() : "";
    }
    public String getFirstYear() {
        List l =  get(getYearId());
        return (l.size() != 0) ? ((TagTextField)l.get(0)).getContent() : "";
    }
    public String getFirstComment() {
        List l =  get(getCommentId());
        return (l.size() != 0) ? ((TagTextField)l.get(0)).getContent() : "";
    }

    
    public void setTitle(String s) {
        set(createTitleField(s));
    }
    public void setAlbum(String s) {
        set(createAlbumField(s));
    }
    public void setArtist(String s) {
        set(createArtistField(s));
    }
    public void setGenre(String s) {
        set(createGenreField(s));
    }
    public void setTrack(String s) {
        set(createTrackField(s));
    }
    public void setYear(String s) {
        set(createYearField(s));
    }
    public void setComment(String s) {
        set(createCommentField(s));
    }
    
    
    public void addTitle(String s) {
        add(createTitleField(s));
    }
    public void addAlbum(String s) {
        add(createAlbumField(s));
    }
    public void addArtist(String s) {
        add(createArtistField(s));
    }
    public void addGenre(String s) {
        add(createGenreField(s));
    }
    public void addTrack(String s) {
        add(createTrackField(s));
    }
    public void addYear(String s) {
        add(createYearField(s));
    }
    public void addComment(String s) {
        add(createCommentField(s));
    }
    
    
    public boolean hasField(String id) {
        return get(id).size() != 0; 
    }
    public boolean isEmpty() {
        return fields.size() == 0;
    }
    public boolean hasCommonFields() {
        return commonNumber != 0;
    }
    
    
    public Iterator getFields() {
        final Iterator it = this.fields.entrySet().iterator();
        return new Iterator() {
            private Iterator fieldsIt;
            
            public boolean hasNext() {
                if(fieldsIt == null) {
                    changeIt();
                }
                return it.hasNext() || (fieldsIt != null && fieldsIt.hasNext());
            }
            
            public Object next() {
                if(!fieldsIt.hasNext())
                    changeIt();
                
                return fieldsIt.next();
            }
            
            private void changeIt() {
                if(!it.hasNext())
                    return;
                
                List l = (List) ((Map.Entry)it.next()).getValue();
                fieldsIt = l.iterator();
            }
            
            public void remove() {
                /* We don't want to remove */
            }
        };
    }
        
    
    public List get(String id) {
		List list = (List) fields.get(id);
		
		if(list == null)
		    return new ArrayList();
		
		return list;
	}
    public void set(TagField field){
		if(field == null)
			return;
		
		//If an empty field is passed, we delete all the previous ones
		if(field.isEmpty()) {
		    Object removed = fields.remove(field.getId());
		    if(removed != null && field.isCommon())
		        commonNumber--;
		    return;
		}
		
		//If there is already an existing field with same id
		//and both are TextFields, we update the first element
		List l = (List) fields.get(field.getId());
		if(l != null) {
		    TagField f = (TagField) l.get(0);
		    f.copyContent(field);
		    return;
		}
		
		//Else we put the new field in the fields.
		l = new ArrayList();
		l.add(field);
		fields.put(field.getId(), l);
		if(field.isCommon())
		    commonNumber++;
	}
    public void add(TagField field) {
		if(field == null || field.isEmpty())
			return;
		
		List list = (List) fields.get(field.getId());
		
		//There was no previous item
		if(list == null) {
		    list = new ArrayList();
		    list.add(field);
		    fields.put(field.getId(), list);
		    if(field.isCommon())
		        commonNumber++;
		}
		else {
			//We append to existing list
			list.add(field);
		}
	}
    
    public String toString() {
		StringBuffer out = new StringBuffer();
		out.append("Tag content:\n");
		Iterator it = getFields();
		while(it.hasNext()) {
		    TagField field = (TagField) it.next();
		    out.append("\t");
		    out.append(field.getId());
		    out.append(" : ");
		    out.append(field.toString());
		    out.append("\n");
		}
		return out.toString().substring(0,out.length()-1);
	}
    
    public void merge(Tag tag) {
        //FIXME: Improve me, for the moment,
        //it overwrites this tag with other values
        //FIXME: TODO: an abstract method that merges particular things for each 
        //format
        if( getTitle().size() == 0)
			setTitle(tag.getFirstTitle());
		if( getArtist().size() == 0 )
		    setArtist(tag.getFirstArtist());
		if( getAlbum().size() == 0 )
		    setAlbum(tag.getFirstAlbum());
		if( getYear().size() == 0 )
		    setYear(tag.getFirstYear());
		if( getComment().size() == 0 )
		    setComment(tag.getFirstComment());
		if( getTrack().size() == 0 )
		    setTrack(tag.getFirstTrack());
		if( getGenre().size() == 0 )
		    setGenre(tag.getFirstGenre());
    }
    
    /*public boolean setEncoding(String enc) {
        if(!isAllowedEncoding(enc)) {
            return false;
        }
        
        Iterator it = getFields();
        while(it.hasNext()) {
            TagField field = (TagField) it.next();
            if(field instanceof TagTextField) {
                ((TagTextField)field).setEncoding(enc);
            }
        }
        
        return true;
    }*/
    //--------------------------------
    protected abstract String getArtistId();
    protected abstract String getAlbumId();
    protected abstract String getTitleId();
    protected abstract String getTrackId();
    protected abstract String getYearId();
    protected abstract String getCommentId();
    protected abstract String getGenreId();
    
    protected abstract TagField createArtistField(String content);
    protected abstract TagField createAlbumField(String content);
    protected abstract TagField createTitleField(String content);
    protected abstract TagField createTrackField(String content);
    protected abstract TagField createYearField(String content);
    protected abstract TagField createCommentField(String content);
    protected abstract TagField createGenreField(String content);
    
    protected abstract boolean isAllowedEncoding(String enc);
    //---------------------------------------
}


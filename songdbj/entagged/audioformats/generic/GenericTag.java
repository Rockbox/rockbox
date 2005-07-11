package entagged.audioformats.generic;


public class GenericTag extends AbstractTag {
    private final static String[] keys = 
	{
        "ARTIST",
        "ALBUM",
        "TITLE",
        "TRACK",
        "YEAR",
        "GENRE",
        "COMMENT",
	};

	public static final int ARTIST = 0;
	public static final int ALBUM = 1;
	public static final int TITLE = 2;
	public static final int TRACK = 3;
	public static final int YEAR = 4;
	public static final int GENRE = 5;
	public static final int COMMENT = 6;
		
	protected String getArtistId() {
	    return keys[ARTIST];
	}
	protected String getAlbumId() {
	    return keys[ALBUM];
	}
	protected String getTitleId() {
	    return keys[TITLE];
	}
	protected String getTrackId() {
	    return keys[TRACK];
	}
	protected String getYearId() {
	    return keys[YEAR];
	}
	protected String getCommentId() {
	    return keys[COMMENT];
	}
	protected String getGenreId() {
	    return keys[GENRE];
	}
	
	protected TagField createArtistField(String content) {
	    return new GenericTagTextField(keys[ARTIST], content);
	}
	protected TagField createAlbumField(String content) {
	    return new GenericTagTextField(keys[ALBUM], content);
	}
	protected TagField createTitleField(String content) {
	    return new GenericTagTextField(keys[TITLE], content);
	}
	protected TagField createTrackField(String content) {
	    return new GenericTagTextField(keys[TRACK], content);
	}
	protected TagField createYearField(String content) {
	    return new GenericTagTextField(keys[YEAR], content);
	}
	protected TagField createCommentField(String content) {
	    return new GenericTagTextField(keys[COMMENT], content);
	}
	protected TagField createGenreField(String content) {
	    return new GenericTagTextField(keys[GENRE], content);
	}
	
	protected boolean isAllowedEncoding(String enc) {
	    return true;
	}
	
	private class GenericTagTextField implements TagTextField {
	    
	    private String id;
	    private String content;
	    
	    public GenericTagTextField(String id, String content) {
	        this.id = id;
	        this.content = content;
	    }
	    
	    public String getContent() {
	        return this.content;
	    }

	    public String getEncoding() {
	        return "ISO-8859-1";
	    }

	    public void setContent(String s) {
	        this.content = s;
	    }

	    public void setEncoding(String s) {
	        /* Not allowed */
	    }
	    
	    public String getId() {
	        return id;
	    }

	    public byte[] getRawContent() {
	        /* FIXME: What to do here ? not supported */
	        return new byte[] {};
	    }

	    public boolean isBinary() {
	        return false;
	    }

	    public void isBinary(boolean b) {
	        /* not supported */
	    }

	    public boolean isCommon() {
	        return true;
	    }

	    public boolean isEmpty() {
	        return this.content.equals("");
	    }
	    
	    public String toString() {
	        return getId() + " : " + getContent();
	    }
	    
	    public void copyContent(TagField field) {
	        if(field instanceof TagTextField) {
	            this.content = ((TagTextField)field).getContent();
	        }
	    }
	}
}

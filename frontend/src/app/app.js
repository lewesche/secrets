import image from "../assets/secrets_diagram_crop.png";


// Sets up the little about section
export const  init = () => {
    let about = document.getElementById('about');
        let title = document.createElement("h2");

        title.innerHTML = "- About";
        title.classList.add("left", "dropDown");
        about.append(title);

    let inner = document.createElement("div");
    
    let p1 = document.createElement("p");
    p1.innerHTML = "Secrets is an app that lets you store strings like passwords in a secure way."
    inner.append(p1);

    let p2 = document.createElement("p");
    p2.innerHTML = "No passwords are saved, and no data is saved in an unencrypted format."
    inner.append(p2);   
    
    let p3 = document.createElement("p");
    p3.innerHTML = "Optionally, secrets can be created with a tag."
    inner.append(p3);   

    let p4 = document.createElement("p");
    p4.innerHTML = "Tags, along with the index, can be used to filter read/delete operations. "
    inner.append(p4);   

    let diagram = document.createElement("img");
    diagram.src=image
    diagram.width = "950";
    inner.append(diagram);

	about.append(inner);

    $(".dropDown").click(function(){
        let curr = this.innerHTML;
        if(curr[0] == '-') {
            curr = curr.replace('-', '+');
        } else if (curr[0] == '+') {
            curr = curr.replace('+', '-');
        }   
        this.innerHTML = curr;
        var oldWidth = $(this.nextSibling).width();
        $(this.nextSibling).slideToggle(200, () => {$(this).width(oldWidth);} );
    }); 

}


